/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorIkApplyRotation.h"
#include "behaviorGraphUtils.inl"
#include "behaviorGraphInstance.h"
#include "skeleton.h"
#include "animatedComponent.h"

IMPLEMENT_ENGINE_CLASS( SApplyRotationIKSolverData );
IMPLEMENT_ENGINE_CLASS( SApplyRotationIKSolver );

#define INDEX_NONE -1

//////////////////////////////////////////////////////////////////////////

SApplyRotationIKSolverData::SApplyRotationIKSolverData()
{
}

//////////////////////////////////////////////////////////////////////////

SApplyRotationIKSolver::SApplyRotationIKSolver()
	: m_parentBone( INDEX_NONE )
	, m_bone( INDEX_NONE )
	, m_startDirMS( AnimVector4::EZ )
	, m_endDirMS( AnimVector4::EZ )
{
}

void SApplyRotationIKSolver::Setup( CBehaviorGraphInstance& instance, const SApplyRotationIKSolverData& data )
{
	const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton();
	if ( ! data.m_bone.Empty() )
	{
		m_bone = skeleton->FindBoneByName( data.m_bone );
	}
	else
	{
		m_bone = INDEX_NONE;
	}

	m_parentBone = m_bone != INDEX_NONE? skeleton->GetParentBoneIndex( m_bone ) : INDEX_NONE;
}

void SApplyRotationIKSolver::StorePreIK( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output )
{
	m_preIKTMS = output.GetBoneModelTransform( instance.GetAnimatedComponent(), m_bone );
}

void SApplyRotationIKSolver::SetAdjustment( AnimVector4 const & startDirMS, AnimVector4 const & endDirMS )
{
	m_startDirMS = startDirMS;
	m_endDirMS = endDirMS;
}

void SApplyRotationIKSolver::UpdatePose( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const SApplyRotationIKSolverData& data, Float weight, Bool updateTranslationToKeepDir, Float additionalOffsetAlongEndDir )
{
	if ( m_bone == INDEX_NONE  )
	{
		return;
	}

	// get parent and bone from output in model space
	AnimQsTransform parentBoneTMS = m_parentBone == INDEX_NONE? AnimQsTransform::IDENTITY : output.GetBoneModelTransform( instance.GetAnimatedComponent(), m_parentBone );
	AnimQsTransform boneTMS;
	SetMulTransform( boneTMS, parentBoneTMS, output.m_outputPose[ m_bone ] );

	// calculate rotation to apply with weight applied
	AnimQuaternion applyRotationMS;
	applyRotationMS.SetShortestRotation( m_startDirMS, m_endDirMS );

	// get bone's orientation with applied rotation and store it in our working bone transform
	AnimQuaternion outBoneQMS;
	outBoneQMS.SetMul( applyRotationMS, m_preIKTMS.GetRotation() ); // apply rotation to pre IK rotation
	AnimQsTransform outBoneTMS = boneTMS; // but get everything else from bone we got in output - other IK could modify translation (well, it will do it in 99.9% of cases)
	outBoneTMS.SetRotation( outBoneQMS );

	if ( updateTranslationToKeepDir )
	{
		Float distAlongStartDirMS = RedMath::SIMD::Dot3( m_preIKTMS.GetTranslation(), m_startDirMS );
		Float distAlongEndDirMS = RedMath::SIMD::Dot3( outBoneTMS.GetTranslation(), m_endDirMS );
		// if both distances don't match, move along startDirMS amount that is desired to end with distAlongEndDirMS same as distAlongStartDirMS
		Float startEndDot = RedMath::SIMD::Dot3( m_startDirMS, m_endDirMS );
		Float moveAlong = startEndDot != 0.0f? ( distAlongStartDirMS - distAlongEndDirMS ) / startEndDot : 0.0f;
		moveAlong = Clamp( moveAlong, -1.0f, 1.0f );
		outBoneTMS.SetTranslation( Add( outBoneTMS.GetTranslation(), Mul( m_startDirMS, moveAlong ) ) );
	}

	if ( additionalOffsetAlongEndDir != 0.0f )
	{
		outBoneTMS.SetTranslation( Add( outBoneTMS.GetTranslation(), Mul( m_endDirMS, additionalOffsetAlongEndDir ) ) );
	}

	// blend to apply weight
	BlendTwoTransforms( outBoneTMS, boneTMS, outBoneTMS, weight );

	// to output's local
	AnimQsTransform& outputBone = output.m_outputPose[ m_bone ];
	outputBone.SetMulInverseMul( parentBoneTMS, outBoneTMS );
}

//////////////////////////////////////////////////////////////////////////

#undef INDEX_NONE

