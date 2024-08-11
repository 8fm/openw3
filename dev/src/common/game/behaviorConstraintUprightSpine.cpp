/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorConstraintUprightSpine.h"
#include "../engine/behaviorGraphInstance.h"
#include "../engine/behaviorGraphUtils.inl"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/skeleton.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

#define INDEX_NONE -1

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintUprightSpineBonesData );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintUprightSpineBones );
IMPLEMENT_ENGINE_CLASS( CBehaviorConstraintUprightSpine );

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( torso3 );

RED_DEFINE_STATIC_NAME( l_bicep );
RED_DEFINE_STATIC_NAME( l_forearm );
RED_DEFINE_STATIC_NAME( l_hand );
RED_DEFINE_STATIC_NAME( r_bicep );
RED_DEFINE_STATIC_NAME( r_forearm );
RED_DEFINE_STATIC_NAME( r_hand );

RED_DEFINE_STATIC_NAME( LeftHand );
RED_DEFINE_STATIC_NAME( RightHand );

RED_DEFINE_STATIC_NAME( HandsIkOff );
RED_DEFINE_STATIC_NAME( LeftHandIkOff );
RED_DEFINE_STATIC_NAME( RightHandIkOff );

//////////////////////////////////////////////////////////////////////////

SBehaviorConstraintUprightSpineBonesData::SBehaviorConstraintUprightSpineBonesData()
: m_boneName( CNAME( torso3 ) )
, m_weight( 1.1f )
, m_weightMatchEntity( 0.6f )
, m_weightMatchEntityFullSpeed( 0.3f )
, m_boneCount( 3 )
{
}

//////////////////////////////////////////////////////////////////////////

void SBehaviorConstraintUprightSpineBones::Setup( CBehaviorGraphInstance& instance, const SBehaviorConstraintUprightSpineBonesData & data )
{
	m_weightPerBone = 0.0f;
	m_weightMatchEntityPerBone = 0.0f;
	m_weightMatchEntityFullSpeedPerBone = 0.0f;
	const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton();
	Int32 boneIdx = skeleton->FindBoneByName( data.m_boneName );
	if ( boneIdx != INDEX_NONE )
	{
		m_boneIndices.PushBack( boneIdx );
		for (Int32 bone = 0; bone < data.m_boneCount; ++ bone )
		{
			boneIdx = skeleton->GetParentBoneIndex( boneIdx );
			if ( boneIdx != INDEX_NONE )
			{
				m_boneIndices.PushBack( boneIdx );
			}
			else
			{
				break;
			}
		}
		if ( m_boneIndices.Size() > 1 )
		{
			m_weightPerBone = data.m_weight / ( ((Float)m_boneIndices.Size()) - 1.0f );
			m_weightMatchEntityPerBone = data.m_weightMatchEntity / ( ((Float)m_boneIndices.Size()) - 1.0f );
			m_weightMatchEntityFullSpeedPerBone = data.m_weightMatchEntityFullSpeed / ( ((Float)m_boneIndices.Size()) - 1.0f );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CBehaviorConstraintUprightSpine::CBehaviorConstraintUprightSpine()
{
	m_leftHandIK.m_upperBone.m_name = CNAME( l_bicep );
	m_leftHandIK.m_jointBone.m_name = CNAME( l_forearm );
	m_leftHandIK.m_lowerBone.m_name = CNAME( l_hand );
	m_leftHandIK.m_autoSetupDirs = false;
	m_leftHandIK.m_jointSideWeightUpper = 1.0f;
	m_leftHandIK.m_jointSideWeightJoint = 0.1f;
	m_leftHandIK.m_jointSideWeightLower = 0.1f;
	m_leftHandIK.m_sideDirUpperBS = Vector( 0.0f,  0.0f,  1.0f );
	m_leftHandIK.m_sideDirJointBS = Vector( 0.0f, -1.0f,  0.0f );
	m_leftHandIK.m_sideDirLowerBS = Vector( 0.0f, -1.0f,  0.0f );
	m_leftHandIK.m_bendDirUpperBS = Vector( 0.0f,  1.0f,  0.0f );
	m_leftHandIK.m_bendDirJointBS = Vector( 0.0f,  0.0f,  1.0f );
	m_leftHandIK.m_bendDirLowerBS = Vector( 0.0f,  0.0f,  1.0f );

	m_rightHandIK.m_upperBone.m_name = CNAME( r_bicep );
	m_rightHandIK.m_jointBone.m_name = CNAME( r_forearm );
	m_rightHandIK.m_lowerBone.m_name = CNAME( r_hand );
	m_rightHandIK.m_autoSetupDirs = false;
	m_rightHandIK.m_jointSideWeightUpper = 1.0f;
	m_rightHandIK.m_jointSideWeightJoint = 0.1f;
	m_rightHandIK.m_jointSideWeightLower = 0.1f;
	m_rightHandIK.m_sideDirUpperBS = Vector( 0.0f,  0.0f,  1.0f );
	m_rightHandIK.m_sideDirJointBS = Vector( 0.0f,  1.0f,  0.0f );
	m_rightHandIK.m_sideDirLowerBS = Vector( 0.0f,  1.0f,  0.0f );
	m_rightHandIK.m_bendDirUpperBS = Vector( 0.0f,  1.0f,  0.0f );
	m_rightHandIK.m_bendDirJointBS = Vector( 0.0f,  0.0f, -1.0f );
	m_rightHandIK.m_bendDirLowerBS = Vector( 0.0f,  0.0f, -1.0f );

	m_matchEntityFullSpeed = 3.0f;
}

void CBehaviorConstraintUprightSpine::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;
	compiler << i_matchEntity;
	compiler << i_matchEntityLocation;
	compiler << i_matchEntityUseFullSpeed;
	compiler << i_bones;
	compiler << i_leftHandIK;
	compiler << i_rightHandIK;
	compiler << i_handIKTargetWeightHandler;
	compiler << i_leftHandIKTargetWeightHandler;
	compiler << i_rightHandIKTargetWeightHandler;
	compiler << i_handIKWeight;
	compiler << i_leftHandIKWeight;
	compiler << i_rightHandIKWeight;
}

void CBehaviorConstraintUprightSpine::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_weight ] = 0.0f;
	instance[ i_matchEntityUseFullSpeed ] = 0.0f;
	instance[ i_bones ].Setup( instance, m_bones );
	instance[ i_leftHandIK ].Setup( instance, m_leftHandIK );
	instance[ i_rightHandIK ].Setup( instance, m_rightHandIK );
	instance[ i_handIKTargetWeightHandler ].Setup( instance, CNAME( HandsIkOff ) );
	instance[ i_leftHandIKTargetWeightHandler ].Setup( instance, CNAME( LeftHandIkOff ) );
	instance[ i_rightHandIKTargetWeightHandler ].Setup( instance, CNAME( RightHandIkOff ) );
	instance[ i_handIKWeight ] = 0.0f;
	instance[ i_leftHandIKWeight ] = 0.0f;
	instance[ i_rightHandIKWeight ] = 0.0f;
}

void CBehaviorConstraintUprightSpine::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	instance[ i_timeDelta ] = timeDelta;
}

static void PrepareArmIK( STwoBonesIKSolver& handIK, AnimQsTransform & targetHandTMS, const AnimQsTransform & handTMS, const AnimQsTransform & shoulderTMS, const AnimQsTransform & currentShoulderTMS, Float armLengthPt )
{
	Float armLength = handIK.GetLength();
	AnimVector4 currentArm = Sub( targetHandTMS.GetTranslation(), currentShoulderTMS.GetTranslation() );
	Float currentArmDist = currentArm.Length3();
	Float inputArmDist = Sub( handTMS.GetTranslation(), shoulderTMS.GetTranslation() ).Length3();
	Float desiredDist = Min( currentArmDist,
		// limit current arm dist to be not greater than:
		Min(
			armLength * armLengthPt, // shouldn't be longer than [armLengthPt] of default length
			inputArmDist * 1.2f // shouldn't be longer than 120% of distance that came from input
			) );
	targetHandTMS.SetTranslation( Add( currentShoulderTMS.GetTranslation(), Div( Mul( currentArm, desiredDist), currentArmDist ) ) );
}

void CBehaviorConstraintUprightSpine::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );
	
	Float timeDelta = instance[ i_timeDelta ];
	CAnimatedComponent const * ac = instance.GetAnimatedComponent();
	ASSERT(ac);
	CMovingAgentComponent const * mac = Cast<CMovingAgentComponent>( ac );
	CEntity * newMatchEntity = mac? mac->GetAnimationProxy().GetUseEntityForPelvisOffset() : nullptr;
	CMovingAgentComponent const * nmeac = newMatchEntity? Cast< CMovingAgentComponent>( newMatchEntity->GetRootAnimatedComponent() ) : nullptr;

	THandle<CEntity>& matchEntity = instance[ i_matchEntity ];
	// check meac instead of match entity - it will work for horse, but not for boat - boat doesn't have moving agent component
	CMovingAgentComponent const * meac = matchEntity? Cast< CMovingAgentComponent>( matchEntity->GetRootAnimatedComponent() ) : nullptr;

	const Matrix localToWorld = ac->GetEntity()->GetLocalToWorld();

	AnimVector4 aWorldZMS;

	Float targetWeight = 1.0f;
	if ( meac || nmeac )
	{
		targetWeight = matchEntity == newMatchEntity? 1.0f : 0.0f;
	}
	
	if (! meac)
	{
		aWorldZMS = VectorToAnimVector( ac->GetEntity()->GetLocalToWorld().Inverted().TransformVector(Vector(0.0f, 0.0f, 1.0f)) );
		targetWeight = aWorldZMS.SquareLength2() > 0.02f * 0.02f? targetWeight : 0.0f;
	}

	Float & weight = instance[ i_weight ];

	if ( ac )
	{
		Bool nowInCutScene = ac->IsInCinematic();
		if ( const IActorInterface* actor = ac->GetEntity()->QueryActorInterface() ) // TODO - cache it?
		{
#ifdef RED_ASSERTS_ENABLED
			if ( nowInCutScene )
			{
				RED_ASSERT( actor->IsInNonGameplayScene() );
			}
#endif
			nowInCutScene = nowInCutScene || actor->IsInNonGameplayScene();
		}
		if ( nowInCutScene )
		{
			weight = 0.f;
			targetWeight = 0.0f;
		}
	}

	Float & handIKWeight = instance[ i_handIKWeight ];
	Float & leftHandIKWeight = instance[ i_leftHandIKWeight ];
	Float & rightHandIKWeight = instance[ i_rightHandIKWeight ];
	SBehaviorConstraintTargetWeightHandler & handIKTargetWeightHandler = instance[ i_handIKTargetWeightHandler ];
	SBehaviorConstraintTargetWeightHandler & leftHandIKTargetWeightHandler = instance[ i_leftHandIKTargetWeightHandler ];
	SBehaviorConstraintTargetWeightHandler & rightHandIKTargetWeightHandler = instance[ i_rightHandIKTargetWeightHandler ];
	Float targetHandIkWeight = Min(handIKTargetWeightHandler.m_targetWeight, targetWeight);

	weight = BlendOnOffWithSpeedBasedOnTime( weight, targetWeight, 0.5f, timeDelta );
	handIKWeight = Min( weight, BlendOnOffWithSpeedBasedOnTime( handIKWeight, targetHandIkWeight, 0.3f, timeDelta ) );
	leftHandIKWeight = Min( weight, BlendOnOffWithSpeedBasedOnTime( leftHandIKWeight, leftHandIKTargetWeightHandler.m_targetWeight, 0.3f, timeDelta ) );
	rightHandIKWeight = Min( weight, BlendOnOffWithSpeedBasedOnTime( rightHandIKWeight, rightHandIKTargetWeightHandler.m_targetWeight, 0.3f, timeDelta ) );

	handIKTargetWeightHandler.ReadyForNextFrame();

	if ( weight > 0.0f )
	{
		STwoBonesIKSolver& leftHandIK = instance[ i_leftHandIK ];
		STwoBonesIKSolver& rightHandIK = instance[ i_rightHandIK ];

#ifdef DEBUG_TWO_BONES_IK_SOLVER
		leftHandIK.GatherDebugData( instance, output, m_leftHandIK );
		rightHandIK.GatherDebugData( instance, output, m_rightHandIK );
#endif

		AnimQsTransform leftShoulderTMS;
		AnimQsTransform leftHandTMS;
		leftHandIK.GetUpperAndLowerTMS( instance, output, leftShoulderTMS, leftHandTMS );
		AnimQsTransform rightShoulderTMS;
		AnimQsTransform rightHandTMS;
		rightHandIK.GetUpperAndLowerTMS( instance, output, rightShoulderTMS, rightHandTMS );

		AnimQsTransform targetLeftHandTMS = leftHandTMS;	
		AnimQsTransform targetRightHandTMS = rightHandTMS;

		AnimQuaternion beUpright;
		if ( meac )
		{
			Vector & matchEntityLocation = instance[ i_matchEntityLocation ];
			Vector newMatchEntityLocation = meac->GetEntity()->GetLocalToWorld().GetTranslation();
			Float matchEntitySpeed = timeDelta != 0.0f? ( newMatchEntityLocation - matchEntityLocation ).Mag3() / timeDelta : 0.0f;
			Float & matchEntityUseFullSpeed = instance[ i_matchEntityUseFullSpeed ];
			matchEntityUseFullSpeed = BlendToWithBlendTime( matchEntityUseFullSpeed, Clamp( matchEntitySpeed / m_matchEntityFullSpeed, 0.0f, 1.0f ), 0.4f, timeDelta );
			matchEntityLocation = newMatchEntityLocation;

			SAnimationProxyData const & matchap = meac->GetAnimationProxy();
			AnimQuaternion preConstraintsPelvis = matchap.GetPreConstraintsPelvisBoneMS().Rotation;
			AnimQuaternion postConstraintsPelvis = matchap.GetPostConstraintsPelvisBoneMS().Rotation;
			AnimQuaternion invPreConstraintsPelvis;
			invPreConstraintsPelvis.SetInverse( preConstraintsPelvis );
			beUpright = RedQuaternion::Mul(invPreConstraintsPelvis, postConstraintsPelvis );

			SAnimationProxyData const & ap = mac->GetAnimationProxy();
			if ( ap.GetTransform( CNAME( LeftHand ), targetLeftHandTMS ) && 
				ap.GetTransform( CNAME( RightHand ), targetRightHandTMS ) )
			{
				AnimQsTransform invPre;
				invPre.SetInverse( matchap.GetPreConstraintsPelvisBoneMS() );
				targetLeftHandTMS.SetMul( invPre, targetLeftHandTMS );
				targetRightHandTMS.SetMul( invPre, targetRightHandTMS );
				targetLeftHandTMS.SetMul( matchap.GetPostConstraintsPelvisBoneMS(), targetLeftHandTMS );
				targetRightHandTMS.SetMul( matchap.GetPostConstraintsPelvisBoneMS(), targetRightHandTMS );
			}
		}
		else
		{
			beUpright.SetShortestRotation( AnimVector4( 0.0f, 0.0f, 1.0f ), aWorldZMS );
		}

		const SBehaviorConstraintUprightSpineBones & bones = instance[ i_bones ];
		if ( bones.m_boneIndices.Size() > 1 )
		{
			const Int32 * parent = &bones.m_boneIndices[bones.m_boneIndices.Size() - 1];
			const Int32 * bone = parent;
			-- bone;
			const Int32 * endParent = &bones.m_boneIndices[0];

			const Float matchEntityUseFullSpeed = instance[ i_matchEntityUseFullSpeed ];
			Float applyWeight = weight * ( meac? ( bones.m_weightMatchEntityPerBone * (1.0f - matchEntityUseFullSpeed) + matchEntityUseFullSpeed * bones.m_weightMatchEntityFullSpeedPerBone ) : bones.m_weightPerBone );
			AnimQsTransform parentBoneTMS = output.GetBoneModelTransform( ac, *parent );
			for ( ; parent != endParent; -- bone, -- parent )
			{
				AnimQsTransform boneTMS;
				SetMulTransform( boneTMS, parentBoneTMS, output.m_outputPose[ *bone ] );
				AnimQsTransform requestedTMS = boneTMS;
				AnimQuaternion useRotation = AnimQuaternion::Mul( requestedTMS.GetRotation(), beUpright );
				useRotation.Normalize();
				requestedTMS.SetRotation( useRotation );

				BlendTwoTransforms( boneTMS, boneTMS, requestedTMS, applyWeight );

				output.m_outputPose[ *bone ].SetMulInverseMul( parentBoneTMS, boneTMS );

				parentBoneTMS = boneTMS;
			}
		}

		if ( handIKWeight > 0.0f )
		{
			AnimQsTransform currentLeftShoulderTMS;
			leftHandIK.GetUpperTMS( instance, output, currentLeftShoulderTMS );
			AnimQsTransform currentRightShoulderTMS;
			rightHandIK.GetUpperTMS( instance, output, currentRightShoulderTMS );

			PrepareArmIK( leftHandIK, targetLeftHandTMS, leftHandTMS, leftShoulderTMS, currentLeftShoulderTMS, meac? 0.9f : 0.95f );
			PrepareArmIK( rightHandIK, targetRightHandTMS, rightHandTMS, rightShoulderTMS, currentRightShoulderTMS, meac? 0.9f : 0.95f );

			DEBUG_ANIM_TRANSFORM( targetLeftHandTMS );
			DEBUG_ANIM_TRANSFORM( targetRightHandTMS );

			leftHandIK.SetTargetLowerTMS( targetLeftHandTMS );
			rightHandIK.SetTargetLowerTMS( targetRightHandTMS );

			leftHandIK.UpdatePose( instance, output, m_leftHandIK, weight * handIKWeight * leftHandIKWeight, timeDelta );
			rightHandIK.UpdatePose( instance, output, m_rightHandIK, weight * handIKWeight * rightHandIKWeight, timeDelta );
		}
	}
	else
	{
		matchEntity = newMatchEntity;
	}
}

void CBehaviorConstraintUprightSpine::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	instance[ i_handIKTargetWeightHandler ].OnActivated( instance );
	instance[ i_leftHandIKTargetWeightHandler ].OnActivated( instance );
	instance[ i_rightHandIKTargetWeightHandler ].OnActivated( instance );
}

void CBehaviorConstraintUprightSpine::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	instance[ i_handIKTargetWeightHandler ].OnDeactivated( instance );
	instance[ i_leftHandIKTargetWeightHandler ].OnDeactivated( instance );
	instance[ i_rightHandIKTargetWeightHandler ].OnDeactivated( instance );
}

void CBehaviorConstraintUprightSpine::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	TBaseClass::OnGenerateFragments( instance, frame );

	if ( ! m_generateEditorFragments )
	{
		return;
	}

	STwoBonesIKSolver& leftHandIK = instance[ i_leftHandIK ];
	STwoBonesIKSolver& rightHandIK = instance[ i_rightHandIK ];

	leftHandIK.OnGenerateFragments( instance, frame );
	rightHandIK.OnGenerateFragments( instance, frame );
}

#undef INDEX_NONE

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
