
#include "build.h"
#include "controlRigEd.h"

/*
CControlRig::CControlRig( const Setup& setup )
	: m_skeleton( setup.m_skeleton )
{
	m_hipsZ = setup.m_hipsZ; //A_NY;

	FillLegSetup( m_setupLegLeft, setup, true );
	FillLegSetup( m_setupLegRight, setup, false );

	FillArmSetup( m_setupArmLeft, setup, true );
	FillArmSetup( m_setupArmRight, setup, false );

	FillChestSetup( m_setupChest, setup );
}

CControlRig::~CControlRig()
{

}

Bool CControlRig::Solve( const SControlRigSetup& data, SBehaviorGraphOutput& pose )
{
	if ( !m_skeleton )
	{
		return false;
	}

	const CControlRigMapper* mapper = m_skeleton->GetControlRigMapper();
	if ( !mapper )
	{
		return false;
	}

	const Int32 numBones = (Int32)pose.m_numBones;

	const Int32 hipsIdx = mapper->m_hips;
	const Int32 chestOrgin = mapper->m_chestOrigin;
	const Int32 chestEnd = mapper->m_chestEnd;

	if ( hipsIdx == -1 || hipsIdx >= numBones )
	{
		return false;
	}

	Bool shouldCalcIKforLegLeft = false;
	Bool shouldCalcIKforLegRight = false;

	if ( data.m_hipRotation.IsSet() )
	{
		shouldCalcIKforLegLeft = true;
		shouldCalcIKforLegRight = true;
	}

	hkQsTransform& hipsBoneLS = pose.m_outputPose[ hipsIdx ];
	hkQsTransform hipsBoneMS = pose.GetBoneModelTransform( hipsIdx, m_skeleton->GetParentIndices() );

	hkQsTransform spineEndMS;

	if ( data.m_chest.IsSet() )
	{
		ChestIk( hipsBoneMS, pose, data.m_chest, spineEndMS );
	}
	else
	{
		const Int32 spineAIdx = hipsIdx + 1;
		const Int32 spineBIdx = hipsIdx + 2;
		const Int32 spineCIdx = hipsIdx + 3;

		hkQsTransform spineA, spineB;
		
		spineA.setMul( hipsBoneMS, pose.m_outputPose[ spineAIdx ] );
		spineB.setMul( spineA, pose.m_outputPose[ spineBIdx ] );
		spineEndMS.setMul( spineB, pose.m_outputPose[ spineCIdx ] );
	}

	{
		m_setupLegLeft.m_hasFirstJointParentTransformMS = true;
		m_setupLegLeft.m_firstJointParentTransformMS = hipsBoneMS;
		m_setupLegRight.m_hasFirstJointParentTransformMS = true;
		m_setupLegRight.m_firstJointParentTransformMS = hipsBoneMS;

		m_setupLegLeft.m_weight = data.m_leftLeg.m_weight;
		m_setupLegLeft.m_firstJointIkGain = data.m_leftLeg.m_weightFirst;
		m_setupLegLeft.m_secondJointIkGain = data.m_leftLeg.m_weightSecond;
		m_setupLegLeft.m_endJointIkGain = data.m_leftLeg.m_weightEnd;
		m_setupLegLeft.m_enforceEndRotation = data.m_leftLeg.m_enforceRotation;
		m_setupLegLeft.m_cosineMinHingeAngle = data.m_leftLeg.m_cosineMinHingeAngle;
		m_setupLegLeft.m_cosineMaxHingeAngle = data.m_leftLeg.m_cosineMaxHingeAngle;

		m_setupLegRight.m_weight = data.m_rightLeg.m_weight;
		m_setupLegRight.m_firstJointIkGain = data.m_rightLeg.m_weightFirst;
		m_setupLegRight.m_secondJointIkGain = data.m_rightLeg.m_weightSecond;
		m_setupLegRight.m_endJointIkGain = data.m_rightLeg.m_weightEnd;
		m_setupLegRight.m_enforceEndRotation = data.m_rightLeg.m_enforceRotation;
		m_setupLegRight.m_cosineMinHingeAngle = data.m_rightLeg.m_cosineMinHingeAngle;
		m_setupLegRight.m_cosineMaxHingeAngle = data.m_rightLeg.m_cosineMaxHingeAngle;
	}

	// 1. Save MS position for legs
	hkQsTransform legTargetMSLeft( hkQsTransform::IDENTITY );
	hkQsTransform legTargetMSRight( hkQsTransform::IDENTITY );
	if ( shouldCalcIKforLegLeft )
	{
		Ik2Solver::CalcEndBoneMS( m_setupLegLeft, pose, legTargetMSLeft );
	}
	if ( shouldCalcIKforLegRight )
	{
		Ik2Solver::CalcEndBoneMS( m_setupLegRight, pose, legTargetMSRight );
	}

	// 2. Hips
	if ( data.m_hipRotation.IsRotSet() )
	{
		RotateBoneLS( hipsBoneLS, data.m_hipRotation.m_rotationZ, m_hipsZ );
	}
	if ( data.m_hipRotation.IsTransSet() )
	{
		TranslateBoneLS( hipsBoneLS, data.m_hipRotation.m_offsetZ, m_hipsZ );
	}
	if ( shouldCalcIKforLegLeft || shouldCalcIKforLegRight )
	{
		hipsBoneMS = pose.GetBoneModelTransform( hipsIdx, m_skeleton->GetParentIndices() );

		m_setupLegLeft.m_firstJointParentTransformMS = hipsBoneMS;
		m_setupLegRight.m_firstJointParentTransformMS = hipsBoneMS;
	}


	// 3. Restore proper position for legs
	if ( shouldCalcIKforLegLeft )
	{
		m_setupLegLeft.m_endTargetMS = TO_CONST_VECTOR_REF( legTargetMSLeft );
		m_setupLegLeft.m_endTargetRotationMS = legTargetMSLeft.m_rotation;

		Ik2Solver::Solve( m_setupLegLeft, pose );
	}
	if ( shouldCalcIKforLegRight )
	{
		m_setupLegRight.m_endTargetMS = TO_CONST_VECTOR_REF( legTargetMSRight );
		m_setupLegRight.m_endTargetRotationMS = legTargetMSRight.m_rotation;

		Ik2Solver::Solve( m_setupLegRight, pose );
	}

	// 4 and 5. Shoulders and Arms
	{
		static Float gain = 0.2f;

		if ( data.m_rightArm.m_weight > 0.f )
		{
			const Int32 rs = mapper->m_rightShoulder;

			hkQsTransform rightShoulderMS;

			static EAxis axis = A_X;
			static EAxis axisUp = A_X;
			static EAxis axisFwd = A_NZ;

			static Float upA = 45.f;
			static Float downA = -20.f;
			static Float leftA = 45.f;
			static Float rightA = -45.f;

			Float up = DEG2RAD( upA );
			Float down = DEG2RAD( downA );
			Float left = DEG2RAD( leftA );
			Float right = DEG2RAD( rightA );

			{
				LookAtSolver::Setup lookAtSetup;
				lookAtSetup.m_boneDirection = axis;
				lookAtSetup.m_parentMS = spineEndMS;
				lookAtSetup.m_targetMS = data.m_rightArm.m_targetMS;
				lookAtSetup.m_weight = gain;

				lookAtSetup.m_useLimits = true;
				lookAtSetup.m_limitBoneFwdDirection = axisFwd;

				lookAtSetup.m_useRangeLimits = true;
				lookAtSetup.m_limitAngleUp = up;
				lookAtSetup.m_limitAngleDown = down;
				lookAtSetup.m_limitAngleLeft = left;
				lookAtSetup.m_limitAngleRight = right;
				lookAtSetup.m_limitBoneUpDirection = axisUp;

				LookAtSolver::Solve( lookAtSetup, pose.m_outputPose[ rs ], rightShoulderMS );
			}

			m_setupArmRight.m_endTargetMS = data.m_rightArm.m_targetMS;

			m_setupArmRight.m_hasFirstJointParentTransformMS = true;
			m_setupArmRight.m_firstJointParentTransformMS = rightShoulderMS;

			m_setupArmRight.m_weight = data.m_rightArm.m_weight;
			m_setupArmRight.m_firstJointIkGain = data.m_rightArm.m_weightFirst;
			m_setupArmRight.m_secondJointIkGain = data.m_rightArm.m_weightSecond;
			m_setupArmRight.m_endJointIkGain = data.m_rightArm.m_weightEnd;
			m_setupArmRight.m_enforceEndRotation = data.m_rightArm.m_enforceRotation;
			m_setupArmRight.m_cosineMinHingeAngle = data.m_rightLeg.m_cosineMinHingeAngle;
			m_setupArmRight.m_cosineMaxHingeAngle = data.m_rightLeg.m_cosineMaxHingeAngle;

			Ik2Solver::Solve( m_setupArmRight, pose );
		}

		if ( data.m_leftArm.m_weight > 0.f )
		{
			const Int32 ls = mapper->m_leftShoulder;
			
			hkQsTransform leftShoulderMS;

			static EAxis axis = A_X;
			static EAxis axisUp = A_X;
			static EAxis axisFwd = A_Z;

			static Float upA = 45.f;
			static Float downA = -20.f;
			static Float leftA = 45.f;
			static Float rightA = -45.f;

			Float up = DEG2RAD( upA );
			Float down = DEG2RAD( downA );
			Float left = DEG2RAD( leftA );
			Float right = DEG2RAD( rightA );

			{
				LookAtSolver::Setup lookAtSetup;
				lookAtSetup.m_boneDirection = axis;
				lookAtSetup.m_parentMS = spineEndMS;
				lookAtSetup.m_targetMS = data.m_leftArm.m_targetMS;
				lookAtSetup.m_weight = gain;

				lookAtSetup.m_useLimits = true;
				lookAtSetup.m_limitBoneFwdDirection = axisFwd;

				lookAtSetup.m_useRangeLimits = true;
				lookAtSetup.m_limitAngleUp = up;
				lookAtSetup.m_limitAngleDown = down;
				lookAtSetup.m_limitAngleLeft = left;
				lookAtSetup.m_limitAngleRight = right;
				lookAtSetup.m_limitBoneUpDirection = axisUp;

				LookAtSolver::Solve( lookAtSetup, pose.m_outputPose[ ls ], leftShoulderMS );
			}

			m_setupArmLeft.m_endTargetMS = data.m_leftArm.m_targetMS;

			m_setupArmLeft.m_hasFirstJointParentTransformMS = true;
			m_setupArmLeft.m_firstJointParentTransformMS = leftShoulderMS;

			m_setupArmLeft.m_weight = data.m_leftArm.m_weight;
			m_setupArmLeft.m_firstJointIkGain = data.m_leftArm.m_weightFirst;
			m_setupArmLeft.m_secondJointIkGain = data.m_leftArm.m_weightSecond;
			m_setupArmLeft.m_endJointIkGain = data.m_leftArm.m_weightEnd;
			m_setupArmLeft.m_enforceEndRotation = data.m_leftArm.m_enforceRotation;
			m_setupArmLeft.m_cosineMinHingeAngle = data.m_rightLeg.m_cosineMinHingeAngle;
			m_setupArmLeft.m_cosineMaxHingeAngle = data.m_rightLeg.m_cosineMaxHingeAngle;

			Ik2Solver::Solve( m_setupArmLeft, pose );
		}
	}

	return true;
}

void CControlRig::RotateBoneLS( hkQsTransform& bone, Float rotationDeg, EAxis axis ) const
{
	hkQuaternion quat;
	quat.setAxisAngle( BehaviorUtils::hkVectorFromAxis( axis ), DEG2RAD( rotationDeg ) );
	hkQsTransform offset( hkVector4( 0.f, 0.f, 0.f ), quat );
	bone.setMul( bone, offset );
}

void CControlRig::TranslateBoneLS( hkQsTransform& bone, Float translation, EAxis axis ) const
{
	hkVector4 vec = BehaviorUtils::hkVectorFromAxis( axis );
	vec.mul4( translation );
	hkQsTransform offset( vec, hkQuaternion::getIdentity() );
	bone.setMul( bone, offset );
}

void CControlRig::FillLegSetup( Ik2Solver::Setup& ikSetup, const CControlRig::Setup& rigSetup, Bool left ) const
{
	ikSetup.m_skeleton = rigSetup.m_skeleton;

	ikSetup.m_firstJointIdx = left ? rigSetup.m_leftHipIdx : rigSetup.m_rightHipIdx;
	ikSetup.m_secondJointIdx = left ? rigSetup.m_leftKneeIdx : rigSetup.m_rightKneeIdx;
	ikSetup.m_endBoneIdx = left ? rigSetup.m_leftAnkleIdx : rigSetup.m_rightAnkleIdx;

	ikSetup.m_hingeAxisLS = Vector( 0.f, 0.f, 1.f );
}

void CControlRig::FillArmSetup( Ik2Solver::Setup& ikSetup, const Setup& rigSetup, Bool left ) const
{
	ikSetup.m_skeleton = rigSetup.m_skeleton;

	const CControlRigMapper* mapper = ikSetup.m_skeleton->GetControlRigMapper();
	if ( mapper )
	{
		ikSetup.m_firstJointIdx = left ? mapper->m_leftBicep : mapper->m_rightBicep;
		ikSetup.m_secondJointIdx = left ? mapper->m_leftElbow : mapper->m_rightElbow;
		ikSetup.m_endBoneIdx = left ? mapper->m_leftWrist : mapper->m_rightWrist;

		ikSetup.m_hingeAxisLS = Vector( 0.f, 0.f, 1.f );
	}
}

void CControlRig::FillChestSetup( SphericalChainIk3Solver::Setup& ikSetup, const Setup& rigSetup ) const
{
	ikSetup.m_firstJointIdx = rigSetup.m_hipsIdx + 1;
	ikSetup.m_secondJointIdx = rigSetup.m_hipsIdx + 2;
	ikSetup.m_endBoneIdx = rigSetup.m_hipsIdx + 3;
}

Bool CControlRig::ChestIk( const hkQsTransform& pelvisMS, SBehaviorGraphOutput& pose, const SControlRigSetup::SChestIk& chest, hkQsTransform& spineEndMS )
{
	m_setupChest.m_endTargetMS = chest.m_offset;
	m_setupChest.m_firstJointParentTransformMS = pelvisMS;
	m_setupChest.m_weight = chest.m_weight;
	m_setupChest.m_boneDirection1 = chest.m_boneDirection1;
	m_setupChest.m_boneDirection2 = chest.m_boneDirection2;
	m_setupChest.m_boneDirection3 = chest.m_boneDirection3;

	return SphericalChainIk3Solver::Solve( m_setupChest, pose, spineEndMS );
}
*/
