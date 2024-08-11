/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorConstraintStirrups.h"
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

IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintStirrupsCommmonData );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintStirrupsCommmon );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintStirrupData );
IMPLEMENT_ENGINE_CLASS( SBehaviorConstraintStirrup );
IMPLEMENT_ENGINE_CLASS( CBehaviorConstraintStirrups );

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( l_stirrup2 );
RED_DEFINE_STATIC_NAME( r_stirrup2 );

//////////////////////////////////////////////////////////////////////////

void SBehaviorConstraintStirrupsCommmon::Reset()
{
	m_bePerpendicular = 0.0f;
}

void SBehaviorConstraintStirrupsCommmon::Update( CMovingAgentComponent* mac, const SBehaviorConstraintStirrupsCommmonData & data, Float timeDelta )
{
	if ( mac && timeDelta > 0.0f )
	{
		const Matrix localToWorld = mac->GetEntity()->GetLocalToWorld();
		const Vector newLocation = localToWorld.GetTranslation();
		const Vector velocity = ( newLocation - m_prevLocation ) / timeDelta;
		const Float speed = velocity.Mag3();

		const Float targetBePerpendicular = data.m_speedForPerpendicular != 0.0f? Clamp( speed / data.m_speedForPerpendicular, 0.0f, 1.0f ) : 1.0f;

		m_bePerpendicular = BlendToWithBlendTime( m_bePerpendicular, targetBePerpendicular, 0.3f, timeDelta );

		m_prevLocation = newLocation;
	}
}

//////////////////////////////////////////////////////////////////////////

SBehaviorConstraintStirrup::SBehaviorConstraintStirrup()
{
}

void SBehaviorConstraintStirrup::Setup( CBehaviorGraphInstance& instance, const SBehaviorConstraintStirrupData & data )
{
	m_weight = 0.0f;
	m_stirrupBoneIdx = INDEX_NONE;
	if ( const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton() )
	{
		m_stirrupBoneIdx = skeleton->FindBoneByName( data.m_stirrupBoneName );
		m_stirrupParentBoneIdx = skeleton->GetParentBoneIndex( m_stirrupBoneIdx );
		m_stirrupGrandParentBoneIdx = skeleton->GetParentBoneIndex( m_stirrupParentBoneIdx );
	}
}

void SBehaviorConstraintStirrup::UpdateAndSample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, CMovingAgentComponent* mac, Bool blendOut, const SBehaviorConstraintStirrupData & data, const SBehaviorConstraintStirrupsCommmonData & commonData, const SBehaviorConstraintStirrupsCommmon & common, Float timeDelta )
{
	if ( mac &&
		 m_stirrupBoneIdx != INDEX_NONE &&
		 m_stirrupParentBoneIdx != INDEX_NONE &&
		 m_stirrupGrandParentBoneIdx != INDEX_NONE )
	{
		Float targetWeight = blendOut? 0.0f : 1.0f;
		m_weight = BlendOnOffWithSpeedBasedOnTime( m_weight, targetWeight, 0.2f, timeDelta );

		if ( m_weight > 0.0f )
		{
			AnimQsTransform stirrupGrandParentTMS;
			AnimQsTransform stirrupParentTMS;
			AnimQsTransform stirrupTMS;
			stirrupGrandParentTMS = output.GetBoneModelTransform( mac, m_stirrupGrandParentBoneIdx );
			SetMulTransform( stirrupParentTMS, stirrupGrandParentTMS, output.m_outputPose[ m_stirrupParentBoneIdx ] );
			SetMulTransform( stirrupTMS, stirrupParentTMS, output.m_outputPose[ m_stirrupBoneIdx ] );

			AnimQsTransform rootTMS = output.GetBoneModelTransform( mac, 0 );
			AnimMatrix44 rootMMS = rootTMS.ConvertToMatrix();

			// hardcoded axes!
			AnimVector4 downDirMS = rootMMS.GetAxisZ().Normalized3().Negated();

			// calculate quaternion
			AnimQuaternion bePerpendicular;
			bePerpendicular.SetShortestRotation( downDirMS, AnimVector4( 0.0f, 0.0f, -1.0f ) );

			// calculate rotated root
			AnimQsTransform requestedStirrupParentTMS = stirrupParentTMS;
			AnimQuaternion useRotation = AnimQuaternion::Mul( bePerpendicular, stirrupParentTMS.GetRotation() );
			useRotation.Normalize();
			requestedStirrupParentTMS.SetRotation( useRotation );

			BlendTwoTransforms( stirrupParentTMS, stirrupParentTMS, requestedStirrupParentTMS, ( 1.0f - common.m_bePerpendicular ) * commonData.m_weight * m_weight );

			SetMulTransform( stirrupTMS, stirrupParentTMS, output.m_outputPose[ m_stirrupBoneIdx ] );

			SetMulInverseMulTransform( output.m_outputPose[ m_stirrupParentBoneIdx ], stirrupGrandParentTMS, stirrupParentTMS );
			SetMulInverseMulTransform( output.m_outputPose[ m_stirrupBoneIdx ], stirrupParentTMS, stirrupTMS );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CBehaviorConstraintStirrups::CBehaviorConstraintStirrups()
{
	m_common.m_weight = 1.0f;
	m_common.m_speedForPerpendicular = 4.5f;
	m_left.m_stirrupBoneName = CNAME( l_stirrup2 );
	m_right.m_stirrupBoneName = CNAME( r_stirrup2 );
}

void CBehaviorConstraintStirrups::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;
	compiler << i_left;
	compiler << i_right;
	compiler << i_common;
}

void CBehaviorConstraintStirrups::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_left ].Setup( instance, m_left );
	instance[ i_right ].Setup( instance, m_right );
	instance[ i_common ].Reset();
}

void CBehaviorConstraintStirrups::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	instance[ i_timeDelta ] = timeDelta;
}

void CBehaviorConstraintStirrups::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	Float timeDelta = instance[ i_timeDelta ];

	SBehaviorConstraintStirrup & left = instance[ i_left ];
	SBehaviorConstraintStirrup & right = instance[ i_right ];
	SBehaviorConstraintStirrupsCommmon & common = instance[ i_common ];

	CAnimatedComponent * ac = instance.GetAnimatedComponentUnsafe();
	CMovingAgentComponent * mac = Cast<CMovingAgentComponent>( ac );

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

	common.Update( mac, m_common, timeDelta );
	left.UpdateAndSample( context, instance, output, mac, blendOut, m_left, m_common, common, timeDelta );
	right.UpdateAndSample( context, instance, output, mac, blendOut, m_right, m_common, common, timeDelta );
}

//////////////////////////////////////////////////////////////////////////

#undef INDEX_NONE

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
