/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorConstraintMoveHandsByOffset.h"
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

IMPLEMENT_ENGINE_CLASS( CBehaviorConstraintMoveHandsByOffset );

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( l_shoulder );
RED_DEFINE_STATIC_NAME( l_bicep );
RED_DEFINE_STATIC_NAME( l_forearm );
RED_DEFINE_STATIC_NAME( l_hand );
RED_DEFINE_STATIC_NAME( r_shoulder );
RED_DEFINE_STATIC_NAME( r_bicep );
RED_DEFINE_STATIC_NAME( r_forearm );
RED_DEFINE_STATIC_NAME( r_hand );

//////////////////////////////////////////////////////////////////////////

CBehaviorConstraintMoveHandsByOffset::CBehaviorConstraintMoveHandsByOffset()
{
	m_leftHand.m_upperBone.m_name = CNAME( l_bicep );
	m_leftHand.m_jointBone.m_name = CNAME( l_forearm );
	m_leftHand.m_lowerBone.m_name = CNAME( l_hand );
	m_leftHand.m_autoSetupDirs = false;
	m_leftHand.m_jointSideWeightUpper = 1.0f;
	m_leftHand.m_jointSideWeightJoint = 0.1f;
	m_leftHand.m_jointSideWeightLower = 0.1f;
	m_leftHand.m_sideDirUpperBS = Vector( 0.0f,  0.0f,  1.0f );
	m_leftHand.m_sideDirJointBS = Vector( 0.0f, -1.0f,  0.0f );
	m_leftHand.m_sideDirLowerBS = Vector( 0.0f, -1.0f,  0.0f );
	m_leftHand.m_bendDirUpperBS = Vector( 0.0f,  1.0f,  0.0f );
	m_leftHand.m_bendDirJointBS = Vector( 0.0f,  0.0f,  1.0f );
	m_leftHand.m_bendDirLowerBS = Vector( 0.0f,  0.0f,  1.0f );

	m_rightHand.m_upperBone.m_name = CNAME( r_bicep );
	m_rightHand.m_jointBone.m_name = CNAME( r_forearm );
	m_rightHand.m_lowerBone.m_name = CNAME( r_hand );
	m_rightHand.m_autoSetupDirs = false;
	m_rightHand.m_jointSideWeightUpper = 1.0f;
	m_rightHand.m_jointSideWeightJoint = 0.1f;
	m_rightHand.m_jointSideWeightLower = 0.1f;
	m_rightHand.m_sideDirUpperBS = Vector( 0.0f,  0.0f,  1.0f );
	m_rightHand.m_sideDirJointBS = Vector( 0.0f,  1.0f,  0.0f );
	m_rightHand.m_sideDirLowerBS = Vector( 0.0f,  1.0f,  0.0f );
	m_rightHand.m_bendDirUpperBS = Vector( 0.0f,  1.0f,  0.0f );
	m_rightHand.m_bendDirJointBS = Vector( 0.0f,  0.0f, -1.0f );
	m_rightHand.m_bendDirLowerBS = Vector( 0.0f,  0.0f, -1.0f );
}

void CBehaviorConstraintMoveHandsByOffset::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;
	compiler << i_weight;
	compiler << i_leftHandOffset;
	compiler << i_rightHandOffset;
	compiler << i_leftHand;
	compiler << i_rightHand;
}

void CBehaviorConstraintMoveHandsByOffset::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_timeDelta ] = 0.0f;
	instance[ i_weight ] = 0.0f;
	instance[ i_leftHandOffset ] = 0.0f;
	instance[ i_rightHandOffset ] = 0.0f;
	instance[ i_leftHand ].Setup( instance, m_leftHand );
	instance[ i_rightHand ].Setup( instance, m_rightHand );
}

void CBehaviorConstraintMoveHandsByOffset::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	instance[ i_timeDelta ] = timeDelta;

	Float targetWeight = 0.0f;
	if ( CMovingAgentComponent const * mac = Cast< CMovingAgentComponent >( instance.GetAnimatedComponent() ) )
	{
		SAnimationProxyData const & ap = mac->GetAnimationProxy();
		targetWeight = ap.GetUseHandsIK()? 1.0f : 0.0f;
		ap.GetHandsIKOffsets( instance[ i_leftHandOffset ], instance[ i_rightHandOffset ] );
	}

	Float & weight = instance[ i_weight ];
	weight = BlendOnOffWithSpeedBasedOnTime( weight, targetWeight, 0.2f, timeDelta );
}

void CBehaviorConstraintMoveHandsByOffset::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	Float timeDelta = instance[ i_timeDelta ];
	Float weight = instance[ i_weight ];

	STwoBonesIKSolver& leftHand = instance[ i_leftHand ];
	STwoBonesIKSolver& rightHand = instance[ i_rightHand ];

	if ( weight > 0.0f )
	{
		AnimQsTransform leftHandTMS, rightHandTMS;
		Float const leftHandOffset = instance[ i_leftHandOffset ];
		Float const rightHandOffset = instance[ i_rightHandOffset ];
		leftHand.GetLowerTMS( instance, output, leftHandTMS );
		rightHand.GetLowerTMS( instance, output, rightHandTMS );
		leftHandTMS.SetTranslation( Add( leftHandTMS.GetTranslation(), AnimVector4( 0.0f, 0.0f, leftHandOffset, 0.0f ) ) );
		rightHandTMS.SetTranslation( Add( rightHandTMS.GetTranslation(), AnimVector4( 0.0f, 0.0f, rightHandOffset, 0.0f ) ) );

		DEBUG_ANIM_TRANSFORM( leftHandTMS );
		DEBUG_ANIM_TRANSFORM( rightHandTMS );

		leftHand.SetTargetLowerTMS( leftHandTMS );
		rightHand.SetTargetLowerTMS( rightHandTMS );

		// always update, not only when offset is non zero, as IK may slightly modify bones and we would have some jumping/discontinuity on offset getting to or off zero
		// weight = 0.0 is better way to cut it out
		leftHand.UpdatePose( instance, output, m_leftHand, weight, timeDelta );
		rightHand.UpdatePose( instance, output, m_rightHand, weight, timeDelta );
	}
}

void CBehaviorConstraintMoveHandsByOffset::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	instance[ i_weight ] = 0.0f;
}

//////////////////////////////////////////////////////////////////////////

#undef INDEX_NONE

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
