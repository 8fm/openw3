
#include "build.h"
#include "controlRigSolver.h"
#include "controlRig.h"
#include "controlRigPropertySet.h"
#include "renderFrame.h"

void TCrSolver::Solve( TCrInstance* cr )
{
	//cr->m_poseSyncLS = false;

	// 4 and 5. Shoulders and Arms
	{
		const Float gain = cr->m_propertySet->m_shoulderWeight;

		const Float handRWPos = cr->m_effectorsPos[ TCrEffector_HandR ];
		const Float handLWPos = cr->m_effectorsPos[ TCrEffector_HandL ];

		const Float handRWRot = cr->m_effectorsRot[ TCrEffector_HandR ];
		const Float handLWRot = cr->m_effectorsRot[ TCrEffector_HandL ];

		const Float upA = cr->m_propertySet->m_shoulderLimitUpDeg;
		const Float downA = cr->m_propertySet->m_shoulderLimitDownDeg;
		const Float leftA = cr->m_propertySet->m_shoulderLimitLeftDeg;
		const Float rightA = cr->m_propertySet->m_shoulderLimitRightDeg;

		const Float up = DEG2RAD( upA );
		const Float down = DEG2RAD( downA );
		const Float left = DEG2RAD( leftA );
		const Float right = DEG2RAD( rightA );

		static Bool SHOULDER = false;

		if ( handRWPos > 0.f )
		{
			static EAxis axis = A_X;
			static EAxis axisUp = A_X;
			static EAxis axisFwd = A_NZ;

			if ( SHOULDER )
			{
				LookAtSolver::Setup lookAtSetup;
				lookAtSetup.m_boneDirection = axis;
				lookAtSetup.m_parentMS = cr->m_bonesMS[ TCrB_Torso3 ];
				lookAtSetup.m_targetMS = reinterpret_cast< const Vector& >( cr->m_effectors[ TCrEffector_HandR ].Translation );
				lookAtSetup.m_weight = Max( gain * ( handRWPos - 0.5f ), 0.f );

				lookAtSetup.m_useLimits = true;
				lookAtSetup.m_limitBoneFwdDirection = axisFwd;

				lookAtSetup.m_useRangeLimits = true;
				lookAtSetup.m_limitAngleUp = up;
				lookAtSetup.m_limitAngleDown = down;
				lookAtSetup.m_limitAngleLeft = left;
				lookAtSetup.m_limitAngleRight = right;
				lookAtSetup.m_limitBoneUpDirection = axisUp;

				LookAtSolver::Solve( lookAtSetup, cr->m_bonesLS[ TCrB_ShoulderR ], cr->m_bonesMS[ TCrB_ShoulderR ] );
			}

			Ik2Solver::Input& ik2Input = m_handR_ik2Input;
			Ik2Solver::Output& ik2Output = m_handR_ik2Output;

			ik2Input.m_endTargetMS = cr->m_effectors[ TCrEffector_HandR ].Translation;

			ik2Input.m_firstJointParentTransformMS = cr->m_bonesMS[ TCrB_ShoulderR ];

			ik2Input.m_firstJointLS = cr->m_bonesLS[ TCrB_BicepR ];
			ik2Input.m_secondJointLS = cr->m_bonesLS[ TCrB_ForearmR ];
			ik2Input.m_endBoneLS = cr->m_bonesLS[ TCrB_HandR ];

			static RedVector4 h( 0.0f, 1.0f, 0.0f );
			ik2Input.m_hingeAxisLS = h;

			ik2Input.m_weight = handRWPos;
			ik2Input.m_firstJointIkGain = 1.f;
			ik2Input.m_secondJointIkGain = 1.f;
			ik2Input.m_endJointIkGain = 1.f;
			
			ik2Input.m_enforceEndRotation = handRWRot > 0.f;
			
			ik2Input.m_endTargetRotationMS = cr->m_bonesMS[ TCrB_WeaponR ].Rotation;// cr->m_effectors[ TCrEffector_HandR ].m_rotation;

			if ( cr->m_weaponOffsetForHandR > 0.f )
			{
				ik2Input.m_endBoneOffsetLS = cr->m_bonesLS[ TCrB_WeaponR ].Translation;
				ik2Input.m_endBoneRotationOffsetLS = cr->m_bonesLS[ TCrB_WeaponR ].Rotation;
			}

			//setupArmRight.m_cosineMinHingeAngle = data.m_rightLeg.m_cosineMinHingeAngle;
			//setupArmRight.m_cosineMaxHingeAngle = data.m_rightLeg.m_cosineMaxHingeAngle;

			Ik2Solver::Solve( ik2Input, ik2Output );

			cr->m_bonesLS[ TCrB_BicepR ] = ik2Output.m_firstJointLS;
			cr->m_bonesLS[ TCrB_ForearmR ] = ik2Output.m_secondJointLS;
			cr->m_bonesLS[ TCrB_HandR ] = ik2Output.m_endBoneLS;
		}

		if ( handLWPos > 0.f )
		{
			static EAxis axis = A_X;
			static EAxis axisUp = A_X;
			static EAxis axisFwd = A_Z;

			if ( SHOULDER )
			{
				LookAtSolver::Setup lookAtSetup;
				lookAtSetup.m_boneDirection = axis;
				lookAtSetup.m_parentMS = cr->m_bonesMS[ TCrB_Torso3 ];
				lookAtSetup.m_targetMS = reinterpret_cast< const Vector& >( cr->m_effectors[ TCrEffector_HandL ].Translation );
				lookAtSetup.m_weight = Max( gain * ( handLWPos - 0.5f ), 0.f );

				lookAtSetup.m_useLimits = true;
				lookAtSetup.m_limitBoneFwdDirection = axisFwd;

				lookAtSetup.m_useRangeLimits = true;
				lookAtSetup.m_limitAngleUp = up;
				lookAtSetup.m_limitAngleDown = down;
				lookAtSetup.m_limitAngleLeft = left;
				lookAtSetup.m_limitAngleRight = right;
				lookAtSetup.m_limitBoneUpDirection = axisUp;

				LookAtSolver::Solve( lookAtSetup, cr->m_bonesLS[ TCrB_ShoulderL ], cr->m_bonesMS[ TCrB_ShoulderL ] );
			}

			Ik2Solver::Input& ik2Input = m_handL_ik2Input;
			Ik2Solver::Output& ik2Output = m_handL_ik2Output;

			ik2Input.m_endTargetMS = cr->m_effectors[ TCrEffector_HandL ].Translation;

			ik2Input.m_firstJointParentTransformMS = cr->m_bonesMS[ TCrB_ShoulderL ];

			ik2Input.m_firstJointLS = cr->m_bonesLS[ TCrB_BicepL ];
			ik2Input.m_secondJointLS = cr->m_bonesLS[ TCrB_ForearmL ];
			ik2Input.m_endBoneLS = cr->m_bonesLS[ TCrB_HandL ];

			static RedVector4 h( 0.0f, -1.0f, 0.0f );
			ik2Input.m_hingeAxisLS = h;

			ik2Input.m_weight = handLWPos;
			ik2Input.m_firstJointIkGain = 1.f;
			ik2Input.m_secondJointIkGain = 1.f;
			ik2Input.m_endJointIkGain = 1.f;

			ik2Input.m_enforceEndRotation = handLWRot > 0.f;
			ik2Input.m_endTargetRotationMS = cr->m_effectors[ TCrEffector_HandL ].Rotation;

			if ( cr->m_weaponOffsetForHandL > 0.f )
			{
				ik2Input.m_endBoneOffsetLS = cr->m_bonesLS[ TCrB_WeaponL ].Translation;
				ik2Input.m_endBoneRotationOffsetLS = cr->m_bonesLS[ TCrB_WeaponL ].Rotation;
			}

			//setupArmRight.m_cosineMinHingeAngle = data.m_rightLeg.m_cosineMinHingeAngle;
			//setupArmRight.m_cosineMaxHingeAngle = data.m_rightLeg.m_cosineMaxHingeAngle;

			Ik2Solver::Solve( ik2Input, ik2Output );

			cr->m_bonesLS[ TCrB_BicepL ] = ik2Output.m_firstJointLS;
			cr->m_bonesLS[ TCrB_ForearmL ] = ik2Output.m_secondJointLS;
			cr->m_bonesLS[ TCrB_HandL ] = ik2Output.m_endBoneLS;
		}
	}
}

void TCrSolver::GenerateFragments( CRenderFrame* frame ) const
{

}
