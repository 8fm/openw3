/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorConstraintPullStirrupsToLegs.h"
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

IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintTargetWeightHandler );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintPullStirrupToLegData );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintPullStirrupToLeg );
IMPLEMENT_ENGINE_CLASS( CBehaviorConstraintPullStirrupsToLegs );

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( l_stirrup2 );
RED_DEFINE_STATIC_NAME( r_stirrup2 );

RED_DEFINE_STATIC_NAME( l_foot );
RED_DEFINE_STATIC_NAME( l_toe );
RED_DEFINE_STATIC_NAME( r_foot );
RED_DEFINE_STATIC_NAME( r_toe );

RED_DEFINE_STATIC_NAME( StirrupsOff );

//////////////////////////////////////////////////////////////////////////

SBehaviorConstraintPullStirrupToLeg::SBehaviorConstraintPullStirrupToLeg()
: m_weight( 0.0f )
, m_distanceTargetWeight( 0.0f )
{
}

void SBehaviorConstraintPullStirrupToLeg::Setup( CBehaviorGraphInstance& instance, const SBehaviorConstraintPullStirrupToLegData & data )
{
	m_weight = 0.0f;
	m_distanceTargetWeight = 0.0f;
	m_stirrupBoneIdx = INDEX_NONE;
	m_footBoneIdx = INDEX_NONE;
}

void SBehaviorConstraintPullStirrupToLeg::UpdateAndSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, CMovingAgentComponent* mac, CMovingAgentComponent* mountMac, Bool blendOut, const SBehaviorConstraintPullStirrupToLegData & data, Float timeDelta, const SBehaviorConstraintTargetWeightHandler & stirrupTargetWeightHandler )
{
	Float targetWeight = mac && mountMac && ! blendOut? 1.0f : 0.0f;

	if ( m_weight == 0.0f && targetWeight != 0.0f )
	{
		if ( m_stirrupBoneIdx == INDEX_NONE )
		{
			ASSERT(mountMac);
			if ( const CSkeleton* mountSkeleton = mountMac->GetSkeleton() )
			{
				m_stirrupBoneIdx = mountSkeleton->FindBoneByName( data.m_stirrupBoneName );
				m_stirrupParentBoneIdx = mountSkeleton->GetParentBoneIndex( m_stirrupBoneIdx );
			}
			if ( m_stirrupBoneIdx == INDEX_NONE ||
				 m_stirrupParentBoneIdx == INDEX_NONE )
			{
				targetWeight = 0.0f;
			}
		}
		if ( m_footBoneIdx == INDEX_NONE )
		{
			ASSERT(mac);
			if ( const CSkeleton* skeleton = mac->GetSkeleton() )
			{
				m_footBoneIdx = skeleton->FindBoneByName( data.m_footBoneName );
				m_toeBoneIdx = skeleton->FindBoneByName( data.m_toeBoneName );
			}
			if ( m_footBoneIdx == INDEX_NONE ||
				 m_toeBoneIdx == INDEX_NONE )
			{
				targetWeight = 0.0f;
			}
		}
	}

	targetWeight = Min( targetWeight, Max( 0.001f, Min( m_distanceTargetWeight, stirrupTargetWeightHandler.m_targetWeight ) ) );
	m_weight = BlendOnOffWithSpeedBasedOnTime( m_weight, targetWeight, 0.2f, timeDelta );

	if ( m_weight > 0.0f && mountMac )
	{
		if ( SBehaviorSampleContext* mountSampleContext = mountMac->GetBehaviorGraphSampleContext() )
		{
			SBehaviorGraphOutput& mountPose = mountSampleContext->GetSampledPose(); // will get constrained
			AnimQsTransform stirrupParentTMS;
			AnimQsTransform stirrupTMS;
			stirrupParentTMS = mountPose.GetBoneModelTransform( mountMac, m_stirrupParentBoneIdx );
			SetMulTransform( stirrupTMS, stirrupParentTMS, mountPose.m_outputPose[ m_stirrupBoneIdx ] );

			AnimQsTransform footTMS = output.GetBoneModelTransform( mac, m_footBoneIdx );
			AnimQsTransform toeTMS = output.GetBoneModelTransform( mac, m_toeBoneIdx );

			AnimQsTransform requestedStirrupTMS = stirrupTMS;
			
			// check axes to calculate how much do we have to rotate
			AnimVector4 requestedAlignSideDirMS;
			TransformVectorNoScale( requestedAlignSideDirMS, toeTMS, VectorToAnimVector( data.m_toeAlignStirrupSideDir ) );

			// hardcoded axes!
			AnimVector4 rotationAxisStirrupMS;// = requestedStirrupTMS.ConvertToMatrix().GetAxisY().Normalized3();
			TransformVectorNoScale( rotationAxisStirrupMS, toeTMS, VectorToAnimVector( data.m_toeAlignStirrupRotationAxisDir ) );
			requestedAlignSideDirMS = Cross( rotationAxisStirrupMS, Cross( requestedAlignSideDirMS, rotationAxisStirrupMS ) ).Normalized3();
			AnimQsTransform requestedStirrupFullyAlignedTMS = requestedStirrupTMS;
			CalculateAnimTransformLFS( requestedStirrupFullyAlignedTMS, requestedStirrupTMS.GetTranslation(), rotationAxisStirrupMS, requestedAlignSideDirMS, AnimVector4(0.0f, 1.0f, 0.0f), AnimVector4(0.0f, 0.0f, 1.0f), AnimVector4(-1.0f, 0.0f, 0.0f) );

			BlendTwoTransforms( requestedStirrupTMS, requestedStirrupTMS, requestedStirrupFullyAlignedTMS, data.m_alignToToeWeight );
			
			// calculate contact points and move stirrup to match contact point
			AnimVector4 requestedContactPointMS;
			TransformLocationNoScale( requestedContactPointMS, footTMS, VectorToAnimVector( data.m_footContactPoint ) );
			AnimVector4 stirrupContactPointMS; // calculate within requested stirrup tms
			TransformLocationNoScale( stirrupContactPointMS, requestedStirrupTMS, VectorToAnimVector( data.m_stirrupContactPoint ) );
		
			// turns out this system (well, stirrup skinning) is much less sophisticated than I thought initially
			requestedStirrupTMS.SetTranslation( Sub( Add( requestedStirrupTMS.GetTranslation(), requestedContactPointMS ), stirrupContactPointMS ) );

			// limit if too far away
			m_distanceTargetWeight = Sub( requestedContactPointMS, stirrupContactPointMS ).SquareLength3() > 0.5f * 0.5f ? 0.0f : 1.0f;

			BlendTwoTransforms( stirrupTMS, stirrupTMS, requestedStirrupTMS, m_weight );
			
			// fill mount pose
			SetMulInverseMulTransform( mountPose.m_outputPose[ m_stirrupBoneIdx ], stirrupParentTMS, stirrupTMS );
		}
	}
	else
	{
		m_stirrupBoneIdx = INDEX_NONE;
		m_footBoneIdx = INDEX_NONE;
	}
}

//////////////////////////////////////////////////////////////////////////

void SBehaviorConstraintTargetWeightHandler::Setup( CBehaviorGraphInstance& instance, const CName & eventName, Bool eventSwitchesOn )
{
	m_eventName = eventName;
	m_eventSwitchesOn = eventSwitchesOn;
	ReadyForNextFrame();
}

void SBehaviorConstraintTargetWeightHandler::OnActivated( CBehaviorGraphInstance& instance )
{
	instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( m_eventName )->RegisterHandler( this );
}

void SBehaviorConstraintTargetWeightHandler::OnDeactivated( CBehaviorGraphInstance& instance )
{
	instance.GetAnimatedComponentUnsafe()->GetAnimationEventNotifier( m_eventName )->UnregisterHandler( this );
}

void SBehaviorConstraintTargetWeightHandler::ReadyForNextFrame()
{
	m_targetWeight = m_eventSwitchesOn? 0.0f : 1.0f;
}

void SBehaviorConstraintTargetWeightHandler::HandleEvent( const CAnimationEventFired &event )
{
	ASSERT( event.GetEventName() == m_eventName );

	m_targetWeight = m_eventSwitchesOn? 1.0f : 0.0f;
}

//////////////////////////////////////////////////////////////////////////

CBehaviorConstraintPullStirrupsToLegs::CBehaviorConstraintPullStirrupsToLegs()
{
	m_leftLeg.m_footBoneName = CNAME( l_foot );
	m_leftLeg.m_stirrupBoneName = CNAME( l_stirrup2 );
	m_leftLeg.m_toeBoneName = CNAME( l_toe );
	m_leftLeg.m_footContactPoint = Vector( 0.12f, -0.00f, 0.0f );
	m_leftLeg.m_stirrupContactPoint = Vector( 0.2f, 0.0f, 0.0f );
	m_leftLeg.m_toeAlignStirrupSideDir = Vector( 0.0f, 0.0f, 1.0f );
	m_leftLeg.m_toeAlignStirrupRotationAxisDir = Vector( 1.0f, -0.4f, 0.0f ).Normalized3();
	m_leftLeg.m_alignToToeWeight = 0.8f;
	m_rightLeg.m_footBoneName = CNAME( r_foot );
	m_rightLeg.m_toeBoneName = CNAME( r_toe );
	m_rightLeg.m_stirrupBoneName = CNAME( r_stirrup2 );
	m_rightLeg.m_footContactPoint = Vector( 0.12f, -0.00f, 0.0f );
	m_rightLeg.m_stirrupContactPoint = Vector( 0.2f, 0.0f, 0.0f );
	m_rightLeg.m_toeAlignStirrupSideDir = Vector( 0.0f, 0.0f, 1.0f );
	m_rightLeg.m_toeAlignStirrupRotationAxisDir = Vector( 1.0f, -0.4f, 0.0f ).Normalized3();
	m_rightLeg.m_alignToToeWeight = 0.8f;
}

void CBehaviorConstraintPullStirrupsToLegs::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;
	compiler << i_leftLeg;
	compiler << i_rightLeg;
	compiler << i_mountEntity;
	compiler << i_stirrupsOffTargetWeightHandler;
}

void CBehaviorConstraintPullStirrupsToLegs::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_leftLeg ].Setup( instance, m_leftLeg );
	instance[ i_rightLeg ].Setup( instance, m_rightLeg );
	instance[ i_mountEntity ] = nullptr;
	instance[ i_stirrupsOffTargetWeightHandler ].Setup( instance, CNAME( StirrupsOff ) );
}

void CBehaviorConstraintPullStirrupsToLegs::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	instance[ i_timeDelta ] = timeDelta;
}

void CBehaviorConstraintPullStirrupsToLegs::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	Float timeDelta = instance[ i_timeDelta ];

	SBehaviorConstraintPullStirrupToLeg & leftLeg = instance[ i_leftLeg ];
	SBehaviorConstraintPullStirrupToLeg & rightLeg = instance[ i_rightLeg ];

	CAnimatedComponent * ac = instance.GetAnimatedComponentUnsafe();
	CMovingAgentComponent * mac = Cast<CMovingAgentComponent>( ac );
	CEntity * newMountEntity = mac? mac->GetAnimationProxy().GetUseEntityForPelvisOffset() : nullptr;
	THandle<CEntity>& mountEntity = instance[ i_mountEntity ];
	Bool blendOut = false;
	if ( newMountEntity != mountEntity )
	{
		if ( ! leftLeg.IsActive() && ! rightLeg.IsActive() )
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

	SBehaviorConstraintTargetWeightHandler & stirrupsOffTargetWeightHandler = instance[ i_stirrupsOffTargetWeightHandler ];
	leftLeg.UpdateAndSample( context, instance, output, mac, mountMac, blendOut, m_leftLeg, timeDelta, stirrupsOffTargetWeightHandler );
	rightLeg.UpdateAndSample( context, instance, output, mac, mountMac, blendOut, m_rightLeg, timeDelta, stirrupsOffTargetWeightHandler );

	stirrupsOffTargetWeightHandler.ReadyForNextFrame();
}

void CBehaviorConstraintPullStirrupsToLegs::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	instance[ i_stirrupsOffTargetWeightHandler ].OnActivated( instance );
}

void CBehaviorConstraintPullStirrupsToLegs::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	instance[ i_stirrupsOffTargetWeightHandler ].OnDeactivated( instance );
}

//////////////////////////////////////////////////////////////////////////

#undef INDEX_NONE

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
