/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorConstraintRiderInSaddle.h"
#include "../engine/behaviorGraphInstance.h"
#include "../engine/behaviorGraphUtils.inl"
#include "../engine/traceTool.h"
#include "../engine/physicsCharacterWrapper.h"
#include "../physics/PhysicsRagdollWrapper.h"
#include "extAnimOnSlopeEvent.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/skeleton.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorConstraintRiderInSaddle );

#define INDEX_NONE -1

//////////////////////////////////////////////////////////////////////////

// RED_DEFINE_STATIC_NAME( pelvis ); <- declared in name registry

//////////////////////////////////////////////////////////////////////////

CBehaviorConstraintRiderInSaddle::CBehaviorConstraintRiderInSaddle()
	: m_bone( CNAME( pelvis ) )
	, m_blendTime( 0.5f )
	, m_blendRotation( 0.5f )
{
}

void CBehaviorConstraintRiderInSaddle::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;
	compiler << i_matchEntity;
	compiler << i_atTargetWeight;
	compiler << i_currentOffset;
	compiler << i_currentRotation;
	compiler << i_bone;
	compiler << i_parentBone;
}

void CBehaviorConstraintRiderInSaddle::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton();

	instance[ i_matchEntity ] = nullptr;
	instance[ i_atTargetWeight ] = 0.0f;
	instance[ i_currentOffset ] = Vector::ZEROS;
	instance[ i_currentRotation ] = Vector( 0.0f, 0.0f, 0.0f, 1.0f );
	instance[ i_bone ] = skeleton->FindBoneByName( m_bone );
	instance[ i_parentBone ] = skeleton->GetParentBoneIndex( instance[ i_bone ] );
}

void CBehaviorConstraintRiderInSaddle::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	instance[ i_matchEntity ] = nullptr;
	instance[ i_atTargetWeight ] = 0.0f;
	instance[ i_currentOffset ] = Vector::ZEROS;
	instance[ i_currentRotation ] = Vector( 0.0f, 0.0f, 0.0f, 1.0f );
}

void CBehaviorConstraintRiderInSaddle::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	instance[ i_timeDelta ] = timeDelta;
}

void CBehaviorConstraintRiderInSaddle::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	Float timeDelta = instance[ i_timeDelta ];
	Int32 bone = instance[ i_bone ];

	CAnimatedComponent const * ac = instance.GetAnimatedComponent();
	CMovingAgentComponent const * mac = Cast<CMovingAgentComponent>( ac );
	CEntity * newMatchEntity = mac? mac->GetAnimationProxy().GetUseEntityForPelvisOffset() : nullptr;

	THandle<CEntity>& matchEntity = instance[ i_matchEntity ];
	Float & atTargetWeight = instance[ i_atTargetWeight ];
	AnimVector4 currentOffset = VectorToAnimVector( instance[ i_currentOffset ] );
	AnimQuaternion currentRotation;
	currentRotation.Quat = VectorToAnimVector( instance[ i_currentRotation ] );
	if ( matchEntity != newMatchEntity )
	{
		matchEntity = newMatchEntity;
		atTargetWeight = 0.0f;
	}

	AnimVector4 targetOffset = AnimVector4::ZEROS;
	AnimQuaternion targetRotation = AnimQuaternion::IDENTITY;
	if ( matchEntity )
	{
		if ( CMovingAgentComponent const * meac = Cast< CMovingAgentComponent>( matchEntity->GetRootAnimatedComponent() ) )
		{
			SAnimationProxyData const & ap = meac->GetAnimationProxy();
			if ( ap.IsPelvisBoneDataValid() && bone != INDEX_NONE )
			{
				// get how
				//		translation of owner-pelvis in pre-constraint other-entity-pelvis
				// changes into
				//		translation of owner-pelvis in post-constraint other-entity-pelvis
				AnimQsTransform prePelvisTMS = output.GetBoneModelTransform( ac, bone );
				AnimQsTransform pelvisToPreTransform;
				pelvisToPreTransform.SetMulInverseMul( ap.GetPreConstraintsPelvisBoneMS(), prePelvisTMS );
				AnimQsTransform postPelvisTMS;
				postPelvisTMS.SetMul( ap.GetPostConstraintsPelvisBoneMS(), pelvisToPreTransform );
				AnimVector4 preToPostTranslation = Sub( postPelvisTMS.GetTranslation(), prePelvisTMS.GetTranslation() );
				targetOffset = preToPostTranslation;
				// and while we're here, get rotation too
				AnimQsTransform preToPostTransform;
				preToPostTransform.SetMulInverseMul( ap.GetPreConstraintsPelvisBoneMS(), ap.GetPostConstraintsPelvisBoneMS() );
				targetRotation = preToPostTransform.GetRotation();
			}
		}
	}

	Float prevAtTargetWeight = atTargetWeight;

	// Instant blend to target offset when in cutscene; smooth blend otherwise

	Bool nowInCutScene = false;
	if ( ac )
	{
		nowInCutScene = ac->IsInCinematic();
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
	}

	atTargetWeight = nowInCutScene ? 1.0f : BlendOnOffWithSpeedBasedOnTime( atTargetWeight, 1.0f, m_blendTime, timeDelta );

	Float useAtTargetWeight = prevAtTargetWeight == 1.0f? 1.0f : Clamp( (atTargetWeight - prevAtTargetWeight) / (1.0f - prevAtTargetWeight), 0.0f, 1.0f );
	currentOffset = Add( Mul( targetOffset, useAtTargetWeight), Mul( currentOffset, 1.0f - useAtTargetWeight ) );
	currentRotation.SetSlerp( currentRotation, targetRotation, useAtTargetWeight );

	// write it back
	instance[ i_currentOffset ] = AnimVectorToVector( currentOffset );
	instance[ i_currentRotation ] = AnimVectorToVector( currentRotation.Quat );

	if ( bone != INDEX_NONE && ( currentOffset.SquareLength3() > 0.0f || ( currentRotation.Quat.SquareLength3() > 0.0f || currentRotation.Quat.W != 1.0f ) ) )
	{
		Int32 parentBone = instance[ i_parentBone ];

		AnimQsTransform parentBoneTMS = output.GetBoneModelTransform( instance.GetAnimatedComponent(), parentBone );
		AnimQsTransform boneTMS;
		boneTMS.SetMul( parentBoneTMS, output.m_outputPose[ bone ] );

		AnimQsTransform outBoneTMS = boneTMS;
		AnimQuaternion outBoneRot = outBoneTMS.GetRotation();
		AnimVector4 outBoneLoc = outBoneTMS.GetTranslation();
		outBoneLoc = Add( outBoneLoc, currentOffset );
		outBoneTMS.SetTranslation( outBoneLoc );
		
		AnimQuaternion fullyRotated;
		fullyRotated.SetMul( outBoneRot, currentRotation );
		outBoneRot.SetSlerp( outBoneRot, fullyRotated, m_blendRotation );
		outBoneTMS.SetRotation( outBoneRot );

		output.m_outputPose[ bone ].SetMulInverseMul( parentBoneTMS, outBoneTMS );
	}
}

#undef INDEX_NONE

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
