/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorConstraintNodeChain.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/animatedComponent.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphConstraintNodeChain );

CBehaviorGraphConstraintNodeChain::CBehaviorGraphConstraintNodeChain()
	: m_solverSteps( 8 )
{

}

void CBehaviorGraphConstraintNodeChain::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_startBoneIndex;
	compiler << i_endBoneIndex;
}

void CBehaviorGraphConstraintNodeChain::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_startBoneIndex ] = FindBoneIndex( m_startBone, instance );
	instance[ i_endBoneIndex ] = FindBoneIndex( m_endBone, instance );
}
#ifdef USE_HAVOK_ANIMATION
hkQsTransform CBehaviorGraphConstraintNodeChain::CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const
#else
RedQsTransform CBehaviorGraphConstraintNodeChain::CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const
#endif
{
	const Int32 boneIndex = instance[ i_endBoneIndex ];
	const CAnimatedComponent* animatedComponent = instance.GetAnimatedComponent();

	if ( boneIndex != -1 && m_targetObject )
	{
#ifdef USE_HAVOK_ANIMATION
		// Calc model transform
		hkQsTransform boneModelTransform = output.GetBoneModelTransform( animatedComponent, boneIndex );

		// Calc new target position
		hkVector4 forward;
		COPY_VECTOR_TO_HK_VECTOR( BehaviorUtils::VectorFromAxis( m_forwardEndBoneDir ), forward );

		forward.setRotatedDir( boneModelTransform.getRotation(), forward );

		// Distance to  target - prev target must have the same distance value because of damping process
		Vector destTargetVec = GetTargetEnd( instance ).GetPosition();
		Float distToTarget = ( destTargetVec - TO_CONST_VECTOR_REF( boneModelTransform.getTranslation() ) ).Mag3();

		forward.mul4( distToTarget );

		forward.add4( boneModelTransform.getTranslation() );

		return hkQsTransform( forward, hkQuaternion::getIdentity(), hkVector4(1,1,1,1) );
	}

	return hkQsTransform::getIdentity();
#else
		// Calc model transform
		RedQsTransform boneModelTransform = output.GetBoneModelTransform( animatedComponent, boneIndex );

		// Calc new target position
		RedVector4 forward;
		forward = BehaviorUtils::RedVectorFromAxis( m_forwardEndBoneDir );

		forward.RotateDirection( boneModelTransform.GetRotation(), forward );

		// Distance to  target - prev target must have the same distance value because of damping process
		RedVector4 destTargetVec = reinterpret_cast< const RedVector4& >( GetTargetEnd( instance ).GetPosition() );
		Float distToTarget = Sub( destTargetVec, boneModelTransform.GetTranslation() ).Length3();

		SetMul(forward, distToTarget);

		SetAdd(forward, boneModelTransform.GetTranslation());

		return RedQsTransform( forward, RedQuaternion::IDENTITY, RedVector4::ONES );
	}

	return RedQsTransform::IDENTITY;
#endif
}

void CBehaviorGraphConstraintNodeChain::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	TBaseClass::Sample( context, instance, output );

	const Int32 startBoneIndex = instance[ i_startBoneIndex ];
	const Int32 endBoneIndex = instance[ i_endBoneIndex ];

	if ( !IsConstraintActive( instance ) || startBoneIndex == -1 || endBoneIndex == -1 ) 
		return;

	CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton();
	if ( skeleton == NULL )
	{
		return;
	}

	SolverData data;
	data.m_startBone = startBoneIndex;
	data.m_endBone = endBoneIndex;
	data.m_solverSteps = m_solverSteps;
	data.m_weight = 1.f;
#ifdef USE_HAVOK_ANIMATION
	data.m_targetMS = GetCurrentConstraintTransform( instance ).getTranslation();
#else
	data.m_targetMS = GetCurrentConstraintTransform( instance ).GetTranslation();
#endif
	Solve( data, skeleton, output );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphConstraintNodeChain::GetCaption() const
{
	if ( !m_name.Empty() )
		return String::Printf( TXT("Chain - %s"), m_name.AsChar() );
	else
		return String( TXT("Chain") );
}

#endif
