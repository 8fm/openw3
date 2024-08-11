/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorConstraintApplyOffset.h"
#include "../engine/behaviorGraphInstance.h"
#include "../engine/behaviorGraphUtils.inl"
#include "../engine/traceTool.h"
#include "../engine/physicsCharacterWrapper.h"
#include "../physics/PhysicsRagdollWrapper.h"
#include "extAnimOnSlopeEvent.h"
#include "../core/instanceDataLayoutCompiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorConstraintApplyOffset );

CBehaviorConstraintApplyOffset::CBehaviorConstraintApplyOffset()
{
}

void CBehaviorConstraintApplyOffset::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;
}

void CBehaviorConstraintApplyOffset::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	instance[ i_timeDelta ] = timeDelta;
}

void CBehaviorConstraintApplyOffset::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	Float timeDelta = instance[ i_timeDelta ];

	CAnimatedComponent * ac = instance.GetAnimatedComponentUnsafe();
	CMovingAgentComponent * mac = Cast<CMovingAgentComponent>( ac );

	if ( mac )
	{
		Float timeLeft = mac->GetAnimationProxy().GetAdditionalOffsetTimeLeft();
		if ( timeLeft > 0.0f )
		{
			Vector const offsetLocMS = mac->GetAnimationProxy().GetAdditionalOffsetLocationMS();
			EulerAngles const offsetRotMS = mac->GetAnimationProxy().GetAdditionalOffsetRotationMS();

			Int32 const rootBoneIdx = 0;
			AnimQsTransform rootMS = output.GetBoneModelTransform( ac, rootBoneIdx );
			AnimVector4 const applyOffsetLocMS = VectorToAnimVector( offsetLocMS );
			AnimQuaternion applyOffsetRotMS; applyOffsetRotMS.Quat = VectorToAnimVector( offsetRotMS.ToQuat() );
			AnimQsTransform const applyOffset = AnimQsTransform( applyOffsetLocMS, applyOffsetRotMS );
			rootMS.SetMul( applyOffset, rootMS );
			output.m_outputPose[ rootBoneIdx ] = rootMS;

			Float const newTimeLeft = Max( 0.0f, timeLeft - timeDelta );
			Float const coefTimeLeft = newTimeLeft / timeLeft; // already checked if timeLeft is > 0.0f
			Vector const newOffsetLocMS = offsetLocMS * coefTimeLeft;
			EulerAngles const newOffsetRotMS = offsetRotMS * coefTimeLeft;
			mac->AccessAnimationProxy().SetAdditionalOffsetMS( newOffsetLocMS, newOffsetRotMS, newTimeLeft );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorConstraintCalcAdSetOffsetForPelvis );

CBehaviorConstraintCalcAdSetOffsetForPelvis::CBehaviorConstraintCalcAdSetOffsetForPelvis()
	: m_pelvisBoneName( TXT("pelvis") )
{

}

void CBehaviorConstraintCalcAdSetOffsetForPelvis::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_pelvisIdx;
}

void CBehaviorConstraintCalcAdSetOffsetForPelvis::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_pelvisIdx ] = FindBoneIndex( m_pelvisBoneName, instance );
}

void CBehaviorConstraintCalcAdSetOffsetForPelvis::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	const Int32 pelvisIdx = instance[ i_pelvisIdx ];
	if ( pelvisIdx != -1 )
	{
		if ( CMovingAgentComponent * mac = Cast< CMovingAgentComponent >( instance.GetAnimatedComponentUnsafe() ) )
		{
			Float duration = 0.f;
			Matrix requestedPelvisWS( Matrix::IDENTITY );

			if ( mac->GetAnimationProxy().HasRequestToApplyPelvisCorrectionWS( requestedPelvisWS, duration ) )
			{
				const Matrix& l2w = mac->GetEntity()->GetLocalToWorld();

				const AnimQsTransform currPelvisTransMS = output.GetBoneModelTransform( mac, pelvisIdx );
				const Matrix currPelvisMS = AnimQsTransformToMatrix( currPelvisTransMS );

				const Matrix requestedPelvisMS = Matrix::Mul( l2w.Inverted(), requestedPelvisWS );

				const Matrix pelvisOffsetLS = Matrix::Mul( requestedPelvisMS.Inverted(), currPelvisMS );

				//const Vector offsetLocMS = pelvisOffsetMS.GetTranslation();
				//const EulerAngles offsetRotMS = pelvisOffsetMS.ToEulerAngles();

				const Vector offsetLocMS = requestedPelvisMS.GetTranslation() - currPelvisMS.GetTranslation();
				const EulerAngles offsetRotMS = EulerAngles::ZEROS;

				mac->AccessAnimationProxy().SetAdditionalOffsetMS( offsetLocMS, offsetRotMS, duration );
				mac->AccessAnimationProxy().ResetRequestToApplyPelvisCorrectionWS();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
