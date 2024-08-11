/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorConstraintPutLegsIntoStirrups.h"
#include "../engine/behaviorGraphInstance.h"
#include "../engine/behaviorGraphUtils.inl"
#include "../core/instanceDataLayoutCompiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

/*
 *					H O W   D O E S   I T    W O R K ?
 *
 *	1.	We can control stirrups on their own, depending on horse speed.
 *		Minor adjustment if stirrups is done to push legs forward when
 *		going slowly down or back when climbing up.
 *	2.	We move each leg to stirrup's space, relatively.
 *		This means that legs follow stirrups on horse.
 *		With this, we still have legs animated, although stirrups
 *		are not on legs.
 *	3.	We move stirrups to match legs if there is rider.
 *	4.	We move stirrups to appear physically correct without rider.
 *
 *		HOW IS IT DONE?
 *
 *	1.	In horse constraint graph we store initial stirrups transforms
 *		and final stirrups transforms.
 *		Initial stirrup transform is without any adjustments, just as
 *		horse is animated.
 *		Final is to know where stirrup ended after adjustments,
 *		but just before adjustment to align with gravity.
 *		Note that before final we could adjust stirrup any way we
 *		would want - but that's not needed right now.
 *	2.	In horse constraint graph after feeding final stirrup transform
 *		we adjust stirrups to appear physically correct without rider.
 *	3.	In rider constraint graph we store initial feet transforms.
 *		We then now how leg was animated before any adjustments.
 *		Therefore we have placement of feet relative to stirrup
 *		as animator imagined it.
 *	4.	In rider constraint graph we move leg to be in stirrup space.
 *		Here we also adjust knee to be a little bit outside to not
 *		clip with horse.
 *		Note that at this point stirrup is not matching foot.
 *		What if we would avoid that? We then would miss legs moving
 *		differently depending on how horse moves and this could
 *		look artificial.
 *	5.	In rider constraint graph we modify HORSE's bones (!)
 *		to put stirrup onto the leg (using contact points and side dirs)
 *		We do this here, as to avoid double pass on constraints.
 *		It is basically a hack. Clean one, but still a hack to avoid
 *		making system more complex and slower to run.
 *		It works because pose constraints are updated always in order
 *		and always right after another.
 */

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintPutLegIntoStirrupData );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintPutLegIntoStirrup );
IMPLEMENT_ENGINE_CLASS( CBehaviorConstraintPutLegsIntoStirrups );

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( LeftFoot );
RED_DEFINE_STATIC_NAME( LeftStirrup );
RED_DEFINE_STATIC_NAME( LeftStirrupFinal );
RED_DEFINE_STATIC_NAME( RightFoot );
RED_DEFINE_STATIC_NAME( RightStirrup );
RED_DEFINE_STATIC_NAME( RightStirrupFinal );

RED_DEFINE_STATIC_NAME( l_thigh );
RED_DEFINE_STATIC_NAME( l_shin );
RED_DEFINE_STATIC_NAME( l_foot );
RED_DEFINE_STATIC_NAME( r_thigh );
RED_DEFINE_STATIC_NAME( r_shin );
RED_DEFINE_STATIC_NAME( r_foot );

RED_DEFINE_STATIC_NAME( StirrupsOff );

//////////////////////////////////////////////////////////////////////////

SBehaviorConstraintPutLegIntoStirrup::SBehaviorConstraintPutLegIntoStirrup()
: m_weight( 0.0f )
{
}

void SBehaviorConstraintPutLegIntoStirrup::Setup( CBehaviorGraphInstance& instance, const SBehaviorConstraintPutLegIntoStirrupData & data )
{
	m_ik.Setup( instance, data.m_ik );
	m_additionalSideDirForIKMS = VectorToAnimVector( data.m_additionalSideDirForIKMS );
}

static void PrepareLegIK( STwoBonesIKSolver& footIK, AnimQsTransform & targetFootTMS, const AnimQsTransform & footTMS, const AnimQsTransform & currentHipTMS, Float legLengthPt )
{
	Float legLength = footIK.GetLength();
	AnimVector4 currentLeg = ( Sub( targetFootTMS.GetTranslation(), currentHipTMS.GetTranslation() ) );
	Float currentLegDist = currentLeg.Length3();
	Float desiredDist = Min( currentLegDist, legLength * legLengthPt );
	targetFootTMS.SetTranslation( Add(currentHipTMS.GetTranslation(), Div(Mul(currentLeg, desiredDist), currentLegDist)) );
}

void SBehaviorConstraintPutLegIntoStirrup::UpdateAndSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, CMovingAgentComponent* mac, CMovingAgentComponent* mountMac, Bool blendOut, const SBehaviorConstraintPutLegIntoStirrupData & data, Float timeDelta, const SBehaviorConstraintTargetWeightHandler & stirrupTargetWeightHandler )
{
	Float targetWeight = 0.0f;

	if ( mac && mountMac )
	{
		if ( mac->GetAnimationProxy().GetTransform( data.m_footStoreName, m_footTransformMS ) &&
			 mountMac->GetAnimationProxy().GetTransform( data.m_stirrupStoreName, m_stirrupTransformMS ) &&
			 mountMac->GetAnimationProxy().GetTransform( data.m_stirrupFinalStoreName, m_stirrupFinalTransformMS ))
		{
			targetWeight = 1.0f;
		}
	}

	targetWeight = blendOut? 0.0f : Min( targetWeight, Max( 0.001f, stirrupTargetWeightHandler.m_targetWeight ) );
	m_weight = BlendOnOffWithSpeedBasedOnTime( m_weight, targetWeight, 0.2f, timeDelta );

	if ( m_weight > 0.0f )
	{
		AnimQsTransform footTransformInStirrupSpace;
		SetMulInverseMulTransform( footTransformInStirrupSpace, m_stirrupTransformMS, m_footTransformMS );
		AnimQsTransform footTransformMS;
		SetMulTransform( footTransformMS, m_stirrupFinalTransformMS, footTransformInStirrupSpace );

		AnimQsTransform hipTMS;
		m_ik.GetUpperTMS( instance, output, hipTMS );

		PrepareLegIK( m_ik, footTransformMS, footTransformMS, hipTMS, 0.95f );

		DEBUG_ANIM_TRANSFORM( footTransformMS );
		m_ik.SetTargetLowerTMS( footTransformMS );
		m_ik.SetAdditionalSideDirMS( m_additionalSideDirForIKMS );
		m_ik.UpdatePose( instance, output, data.m_ik, m_weight, timeDelta );
	}
}

//////////////////////////////////////////////////////////////////////////

CBehaviorConstraintPutLegsIntoStirrups::CBehaviorConstraintPutLegsIntoStirrups()
{
	m_leftLeg.m_footStoreName = CNAME( LeftFoot );
	m_leftLeg.m_stirrupStoreName = CNAME( LeftStirrup );
	m_leftLeg.m_stirrupFinalStoreName = CNAME( LeftStirrupFinal );
	m_rightLeg.m_footStoreName = CNAME( RightFoot );
	m_rightLeg.m_stirrupStoreName = CNAME( RightStirrup );
	m_rightLeg.m_stirrupFinalStoreName = CNAME( RightStirrupFinal );

	m_leftLeg.m_ik.m_upperBone.m_name = CNAME( l_thigh );
	m_leftLeg.m_ik.m_jointBone.m_name = CNAME( l_shin );
	m_leftLeg.m_ik.m_lowerBone.m_name = CNAME( l_foot );
	m_rightLeg.m_ik.m_upperBone.m_name = CNAME( r_thigh );
	m_rightLeg.m_ik.m_jointBone.m_name = CNAME( r_shin );
	m_rightLeg.m_ik.m_lowerBone.m_name = CNAME( r_foot );
	// focus on knee side dir
	m_leftLeg.m_ik.m_jointSideWeightUpper = 0.1f;
	m_leftLeg.m_ik.m_jointSideWeightJoint = 0.8f;
	m_leftLeg.m_ik.m_jointSideWeightLower = 0.1f;
	m_rightLeg.m_ik.m_jointSideWeightUpper = 0.1f;
	m_rightLeg.m_ik.m_jointSideWeightJoint = 0.8f;
	m_rightLeg.m_ik.m_jointSideWeightLower = 0.1f;

	m_leftLeg.m_additionalSideDirForIKMS = Vector( 0.0f, 0.0f, 0.0f );
	m_rightLeg.m_additionalSideDirForIKMS = Vector( 0.0f, 0.0f, 0.0f );
}

void CBehaviorConstraintPutLegsIntoStirrups::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;
	compiler << i_leftLeg;
	compiler << i_rightLeg;
	compiler << i_mountEntity;
	compiler << i_stirrupsOffTargetWeightHandler;
}

void CBehaviorConstraintPutLegsIntoStirrups::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_leftLeg ].Setup( instance, m_leftLeg );
	instance[ i_rightLeg ].Setup( instance, m_rightLeg );
	instance[ i_mountEntity ] = nullptr;
	instance[ i_stirrupsOffTargetWeightHandler ].Setup( instance, CNAME( StirrupsOff ) );
}

void CBehaviorConstraintPutLegsIntoStirrups::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	instance[ i_timeDelta ] = timeDelta;
}

void CBehaviorConstraintPutLegsIntoStirrups::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	Float timeDelta = instance[ i_timeDelta ];

	SBehaviorConstraintPutLegIntoStirrup & leftLeg = instance[ i_leftLeg ];
	SBehaviorConstraintPutLegIntoStirrup & rightLeg = instance[ i_rightLeg ];

	CAnimatedComponent * ac = instance.GetAnimatedComponentUnsafe();
	CMovingAgentComponent * mac = Cast<CMovingAgentComponent>( ac );
	CEntity * newMountEntity = mac? mac->GetAnimationProxy().GetUseEntityForPelvisOffset() : nullptr;
	THandle<CEntity>& mountEntity = instance[ i_mountEntity ];

	if ( newMountEntity != mountEntity )
	{
		if ( ! leftLeg.IsActive() && ! rightLeg.IsActive() )
		{
			mountEntity = newMountEntity;
		}
		else
		{
			// will go down to reset
			mountEntity = nullptr;
		}
	}

	Bool blendOut = false;
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

void CBehaviorConstraintPutLegsIntoStirrups::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	instance[ i_stirrupsOffTargetWeightHandler ].OnActivated( instance );
}

void CBehaviorConstraintPutLegsIntoStirrups::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	instance[ i_stirrupsOffTargetWeightHandler ].OnDeactivated( instance );
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
