/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorConstraintPullReinsToHands.h"
#include "../engine/behaviorGraphInstance.h"
#include "../engine/behaviorGraphUtils.inl"
#include "../engine/behaviorGraphContext.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/skeleton.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

#define INDEX_NONE -1

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintPullReinToHandData );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintPullReinToHand );
IMPLEMENT_ENGINE_CLASS( CBehaviorConstraintPullReinsToHands );

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( dyng_l_rein_hand_IK );
RED_DEFINE_STATIC_NAME( dyng_r_rein_hand_IK );

RED_DEFINE_STATIC_NAME( l_hand );
RED_DEFINE_STATIC_NAME( r_hand );

RED_DEFINE_STATIC_NAME( ReinsOff );
RED_DEFINE_STATIC_NAME( LeftReinOff );
RED_DEFINE_STATIC_NAME( RightReinOff );

RED_DEFINE_STATIC_NAME( ForceReinsOn );
RED_DEFINE_STATIC_NAME( ForceLeftReinOn );
RED_DEFINE_STATIC_NAME( ForceRightReinOn );

//////////////////////////////////////////////////////////////////////////

SBehaviorConstraintPullReinToHand::SBehaviorConstraintPullReinToHand()
: m_weight( 0.0f )
, m_distanceTargetWeight( 0.0f )
{
}

void SBehaviorConstraintPullReinToHand::Setup( CBehaviorGraphInstance& instance, const SBehaviorConstraintPullReinToHandData & data )
{
	m_weight = 0.0f;
	m_distanceTargetWeight = 0.0f;
	m_reinBoneIdx = INDEX_NONE;
	m_handBoneIdx = INDEX_NONE;
}

void SBehaviorConstraintPullReinToHand::UpdateAndSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, CMovingAgentComponent* mac, CMovingAgentComponent* mountMac, Bool blendOut, const SBehaviorConstraintPullReinToHandData & data, Float timeDelta, const SBehaviorConstraintTargetWeightHandler & reinsTargetWeightHandler, const SBehaviorConstraintTargetWeightHandler & reinTargetWeightHandler, const SBehaviorConstraintTargetWeightHandler & forceReinsOnTargetWeightHandler, const SBehaviorConstraintTargetWeightHandler & forceReinOnTargetWeightHandler )
{
	Float targetWeight = mac && mountMac && ! blendOut? 1.0f : 0.0f;

#ifdef NO_EDITOR
	if ( m_weight == 0.0f && targetWeight != 0.0f )
#else
	if ( ( m_weight == 0.0f && targetWeight != 0.0f ) || instance.IsOpenInEditor() )
#endif
	{
		if ( m_reinBoneIdx == INDEX_NONE && mountMac )
		{
			if ( const CSkeleton* mountSkeleton = mountMac->GetSkeleton() )
			{
				m_reinBoneIdx = mountSkeleton->FindBoneByName( data.m_reinBoneName );
				m_reinParentBoneIdx = mountSkeleton->GetParentBoneIndex( m_reinBoneIdx );
			}
			if ( m_reinBoneIdx == INDEX_NONE ||
				 m_reinParentBoneIdx == INDEX_NONE )
			{
				targetWeight = 0.0f;
			}
		}
		if ( m_handBoneIdx == INDEX_NONE && mac )
		{
			if ( const CSkeleton* skeleton = mac->GetSkeleton() )
			{
				m_handBoneIdx = skeleton->FindBoneByName( data.m_handBoneName );
			}
			if ( m_handBoneIdx == INDEX_NONE )
			{
				targetWeight = 0.0f;
			}
		}
	}

#ifndef NO_EDITOR
	m_handTMS = m_handBoneIdx != INDEX_NONE? output.GetBoneModelTransform( instance.GetAnimatedComponent(), m_handBoneIdx ) : AnimQsTransform::IDENTITY;
#endif

	Float forcedWeight = Min(1.0f, forceReinsOnTargetWeightHandler.m_targetWeight + forceReinOnTargetWeightHandler.m_targetWeight);
	targetWeight = Min( targetWeight, Max( 0.001f, Min( m_distanceTargetWeight, reinsTargetWeightHandler.m_targetWeight * reinTargetWeightHandler.m_targetWeight ) ) );
	targetWeight = Max( targetWeight, forcedWeight );
	m_weight = BlendOnOffWithSpeedBasedOnTime( m_weight, targetWeight, 0.2f, timeDelta );

	if ( m_weight > 0.0f && mountMac )
	{
		if ( SBehaviorSampleContext* mountSampleContext = mountMac->GetBehaviorGraphSampleContext() )
		{
			SBehaviorGraphOutput& mountPose = mountSampleContext->GetSampledPose(); // will get constrained
			AnimQsTransform reinParentTMS;
			AnimQsTransform reinTMS;
			reinParentTMS = mountPose.GetBoneModelTransform( mountMac, m_reinParentBoneIdx );
			SetMulTransform( reinTMS, reinParentTMS, mountPose.m_outputPose[ m_reinBoneIdx ] );

			AnimQsTransform handTMS = output.GetBoneModelTransform( mac, m_handBoneIdx );

			AnimQsTransform requestedReinTMS = reinTMS;
			
			// calculate contact points and move rein to match contact point
			AnimVector4 requestedContactPointMS;
			TransformLocationNoScale( requestedContactPointMS, handTMS, VectorToAnimVector( data.m_handContactPoint ) );
			AnimVector4 reinContactPointMS; // calculate within requested rein tms
			TransformLocationNoScale( reinContactPointMS, requestedReinTMS, VectorToAnimVector( data.m_reinContactPoint ) );
		
#ifndef NO_EDITOR
			m_requestedContactPointMS = requestedContactPointMS;
			m_reinContactPointMS = reinContactPointMS;
			m_reinTMS = reinTMS;
#endif

			// turns out this system (well, rein skinning) is much less sophisticated than I thought initially
			requestedReinTMS.SetTranslation( Sub( Add( requestedReinTMS.GetTranslation(), requestedContactPointMS ), reinContactPointMS ) );

			// limit if too far away - but it is better to handle this with anim events
			const Float maxDist = 0.8f;
			m_distanceTargetWeight = Sub( requestedContactPointMS, reinContactPointMS ).SquareLength3() > maxDist * maxDist ? 0.0f : 1.0f;

			BlendTwoTransforms( reinTMS, reinTMS, requestedReinTMS, m_weight );
			
#ifndef NO_EDITOR
			m_postReinTMS = reinTMS;
			TransformLocationNoScale( m_postReinContactPointMS, m_postReinTMS, VectorToAnimVector( data.m_reinContactPoint ) );
#endif

			// fill mount pose
			SetMulInverseMulTransform( mountPose.m_outputPose[ m_reinBoneIdx ], reinParentTMS, reinTMS );
		}
	}
	else
	{
		m_reinBoneIdx = INDEX_NONE;
		m_handBoneIdx = INDEX_NONE;
	}
}

//////////////////////////////////////////////////////////////////////////

CBehaviorConstraintPullReinsToHands::CBehaviorConstraintPullReinsToHands()
{
	m_leftHand.m_handBoneName = CNAME( l_hand );
	m_leftHand.m_reinBoneName = CNAME( dyng_l_rein_hand_IK );
	m_leftHand.m_handContactPoint = Vector( 0.1f, 0.03f, 0.0f );
	m_leftHand.m_reinContactPoint = Vector( 0.0f, 0.0f, 0.0f );
	m_rightHand.m_handBoneName = CNAME( r_hand );
	m_rightHand.m_reinBoneName = CNAME( dyng_r_rein_hand_IK );
	m_rightHand.m_handContactPoint = Vector( 0.1f, 0.03f, 0.0f );
	m_rightHand.m_reinContactPoint = Vector( 0.0f, 0.0f, 0.0f );
}

void CBehaviorConstraintPullReinsToHands::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;
	compiler << i_leftHand;
	compiler << i_rightHand;
	compiler << i_mountEntity;
	compiler << i_reinsOffTargetWeightHandler;
	compiler << i_leftReinOffTargetWeightHandler;
	compiler << i_rightReinOffTargetWeightHandler;
	compiler << i_forceReinsOnTargetWeightHandler;
	compiler << i_forceLeftReinOnTargetWeightHandler;
	compiler << i_forceRightReinOnTargetWeightHandler;
}

void CBehaviorConstraintPullReinsToHands::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_leftHand ].Setup( instance, m_leftHand );
	instance[ i_rightHand ].Setup( instance, m_rightHand );
	instance[ i_mountEntity ] = nullptr;
	instance[ i_reinsOffTargetWeightHandler ].Setup( instance, CNAME( ReinsOff ) );
	instance[ i_leftReinOffTargetWeightHandler ].Setup( instance, CNAME( LeftReinOff ) );
	instance[ i_rightReinOffTargetWeightHandler ].Setup( instance, CNAME( RightReinOff ) );
	instance[ i_forceReinsOnTargetWeightHandler ].Setup( instance, CNAME( ForceReinsOn ), true );
	instance[ i_forceLeftReinOnTargetWeightHandler ].Setup( instance, CNAME( ForceLeftReinOn ), true );
	instance[ i_forceRightReinOnTargetWeightHandler ].Setup( instance, CNAME( ForceRightReinOn ), true );
}

void CBehaviorConstraintPullReinsToHands::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	instance[ i_timeDelta ] = timeDelta;
}

void CBehaviorConstraintPullReinsToHands::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	Float timeDelta = instance[ i_timeDelta ];

	SBehaviorConstraintPullReinToHand & leftHand = instance[ i_leftHand ];
	SBehaviorConstraintPullReinToHand & rightHand = instance[ i_rightHand ];

	CAnimatedComponent * ac = instance.GetAnimatedComponentUnsafe();
	CMovingAgentComponent * mac = Cast<CMovingAgentComponent>( ac );
	CEntity * newMountEntity = mac? mac->GetAnimationProxy().GetUseEntityForPelvisOffset() : nullptr;
	THandle<CEntity>& mountEntity = instance[ i_mountEntity ];
	Bool blendOut = false;
	if ( newMountEntity != mountEntity )
	{
		if ( ! leftHand.IsActive() && ! rightHand.IsActive() )
		{
			mountEntity = newMountEntity;
		}
		else
		{
			// will go down to reset
			blendOut = true;
		}
	}

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
		blendOut |= nowInCutScene;
	}

	CMovingAgentComponent * mountMac = mountEntity? Cast<CMovingAgentComponent>( mountEntity->GetRootAnimatedComponent() ) : nullptr;

	SBehaviorConstraintTargetWeightHandler & reinsOffTargetWeightHandler = instance[ i_reinsOffTargetWeightHandler ];
	SBehaviorConstraintTargetWeightHandler & leftReinOffTargetWeightHandler = instance[ i_leftReinOffTargetWeightHandler ];
	SBehaviorConstraintTargetWeightHandler & rightReinOffTargetWeightHandler = instance[ i_rightReinOffTargetWeightHandler ];
	SBehaviorConstraintTargetWeightHandler & forceReinsOnTargetWeightHandler = instance[ i_forceReinsOnTargetWeightHandler ];
	SBehaviorConstraintTargetWeightHandler & forceLeftReinOnTargetWeightHandler = instance[ i_forceLeftReinOnTargetWeightHandler ];
	SBehaviorConstraintTargetWeightHandler & forceRightReinOnTargetWeightHandler = instance[ i_forceRightReinOnTargetWeightHandler ];
	leftHand.UpdateAndSample( context, instance, output, mac, mountMac, blendOut, m_leftHand, timeDelta, reinsOffTargetWeightHandler, leftReinOffTargetWeightHandler, forceReinsOnTargetWeightHandler, forceLeftReinOnTargetWeightHandler );
	rightHand.UpdateAndSample( context, instance, output, mac, mountMac, blendOut, m_rightHand, timeDelta, reinsOffTargetWeightHandler, rightReinOffTargetWeightHandler, forceReinsOnTargetWeightHandler, forceRightReinOnTargetWeightHandler );

	reinsOffTargetWeightHandler.ReadyForNextFrame();
	leftReinOffTargetWeightHandler.ReadyForNextFrame();
	rightReinOffTargetWeightHandler.ReadyForNextFrame();
	forceReinsOnTargetWeightHandler.ReadyForNextFrame();
	forceLeftReinOnTargetWeightHandler.ReadyForNextFrame();
	forceRightReinOnTargetWeightHandler.ReadyForNextFrame();
}

void CBehaviorConstraintPullReinsToHands::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	instance[ i_reinsOffTargetWeightHandler ].OnActivated( instance );
	instance[ i_leftReinOffTargetWeightHandler ].OnActivated( instance );
	instance[ i_rightReinOffTargetWeightHandler ].OnActivated( instance );
	instance[ i_forceReinsOnTargetWeightHandler ].OnActivated( instance );
	instance[ i_forceLeftReinOnTargetWeightHandler ].OnActivated( instance );
	instance[ i_forceRightReinOnTargetWeightHandler ].OnActivated( instance );
}

void CBehaviorConstraintPullReinsToHands::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	instance[ i_reinsOffTargetWeightHandler ].OnDeactivated( instance );
	instance[ i_leftReinOffTargetWeightHandler ].OnDeactivated( instance );
	instance[ i_rightReinOffTargetWeightHandler ].OnDeactivated( instance );
	instance[ i_forceReinsOnTargetWeightHandler ].OnDeactivated( instance );
	instance[ i_forceLeftReinOnTargetWeightHandler ].OnDeactivated( instance );
	instance[ i_forceRightReinOnTargetWeightHandler ].OnDeactivated( instance );
}

void CBehaviorConstraintPullReinsToHands::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	TBaseClass::OnGenerateFragments( instance, frame );

	if ( ! m_generateEditorFragments )
	{
		return;
	}

#ifndef NO_EDITOR
	Matrix l2w = instance.GetAnimatedComponent()->GetLocalToWorld();
	Matrix leftHandWS = AnimQsTransformToMatrix( instance[i_leftHand].m_handTMS ) * l2w;
	Matrix rightHandWS = AnimQsTransformToMatrix( instance[i_rightHand].m_handTMS ) * l2w;
	Matrix leftReinWS = AnimQsTransformToMatrix( instance[i_leftHand].m_reinTMS ) * l2w;
	Matrix rightReinWS = AnimQsTransformToMatrix( instance[i_rightHand].m_reinTMS ) * l2w;
	Matrix leftPostReinWS = AnimQsTransformToMatrix( instance[i_leftHand].m_postReinTMS ) * l2w;
	Matrix rightPostReinWS = AnimQsTransformToMatrix( instance[i_rightHand].m_postReinTMS ) * l2w;
	DrawDebugMatrix( leftHandWS, 0.05f, 128, frame, Color( 255, 0, 0, 128 ) );
	DrawDebugMatrix( rightHandWS, 0.05f, 128, frame, Color( 255, 0, 0, 128 ) );
	DrawDebugMatrix( leftReinWS, 0.1f, 128, frame, Color( 0, 255, 0, 128 ) );
	DrawDebugMatrix( rightReinWS, 0.1f, 128, frame, Color( 0, 255, 0, 128 ) );
	DrawDebugMatrix( leftPostReinWS, 0.1f, 128, frame, Color( 0, 0, 255, 128 ) );
	DrawDebugMatrix( rightPostReinWS, 0.1f, 128, frame, Color( 0, 0, 255, 128 ) );
	frame->AddDebugLine( leftHandWS.GetTranslation(), leftHandWS.TransformPoint( m_leftHand.m_handContactPoint ), Color(0,255,0,190), true, true);
	frame->AddDebugLine( rightHandWS.GetTranslation(), rightHandWS.TransformPoint( m_rightHand.m_handContactPoint ), Color(0,255,0,190), true, true);
	frame->AddDebugSphere( l2w.TransformPoint( AnimVectorToVector( instance[i_leftHand].m_requestedContactPointMS ) ), 0.02f, Matrix::IDENTITY, Color(255,0,0,200), true, true);
	frame->AddDebugSphere( l2w.TransformPoint( AnimVectorToVector( instance[i_rightHand].m_requestedContactPointMS ) ), 0.02f, Matrix::IDENTITY, Color(255,0,0,200), true, true);
	frame->AddDebugSphere( l2w.TransformPoint( AnimVectorToVector( instance[i_leftHand].m_reinContactPointMS ) ), 0.02f, Matrix::IDENTITY, Color(0,255,0,200), true, true);
	frame->AddDebugSphere( l2w.TransformPoint( AnimVectorToVector( instance[i_rightHand].m_reinContactPointMS ) ), 0.02f, Matrix::IDENTITY, Color(0,255,0,200), true, true);
	frame->AddDebugSphere( l2w.TransformPoint( AnimVectorToVector( instance[i_leftHand].m_postReinContactPointMS ) ), 0.02f, Matrix::IDENTITY, Color(0,0,(Uint8)((Int32)(instance[i_leftHand].m_weight * 255.0f), 0, 255),200), true, true);
	frame->AddDebugSphere( l2w.TransformPoint( AnimVectorToVector( instance[i_rightHand].m_postReinContactPointMS ) ), 0.02f, Matrix::IDENTITY, Color(0,0,(Uint8)((Int32)(instance[i_rightHand].m_weight * 255.0f), 0, 255),200), true, true);
#endif
}

//////////////////////////////////////////////////////////////////////////

#undef INDEX_NONE

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
