/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorConstraintNodeBoneInterpolate.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "behaviorGraphValueNode.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../core/instanceDataLayoutCompiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphConstraintNodeBoneInterpolate );

CBehaviorGraphConstraintNodeBoneInterpolate::CBehaviorGraphConstraintNodeBoneInterpolate()
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphConstraintNodeBoneInterpolate::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );		
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ), true ) );
}

#endif

void CBehaviorGraphConstraintNodeBoneInterpolate::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache input
	m_cachedControlValueNode = CacheValueBlock( TXT("Weight") );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphConstraintNodeBoneInterpolate::GetCaption() const
{
	if ( !m_name.Empty() )
	{
		return String::Printf( TXT("Bone interpolate - %s"), m_name.AsChar() );
	}
	else
	{
		return String( TXT("Bone interpolate") );
	}
}

#endif

void CBehaviorGraphConstraintNodeBoneInterpolate::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphConstraintNodeBoneInterpolate::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Activate( instance );
	}
}

void CBehaviorGraphConstraintNodeBoneInterpolate::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Deactivate( instance );
	}
}

void CBehaviorGraphConstraintNodeBoneInterpolate::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_boneIndexInputA;
	compiler << i_boneIndexInputB;
	compiler << i_boneIndexOutput;
	compiler << i_controlValue;
}

void CBehaviorGraphConstraintNodeBoneInterpolate::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_boneIndexInputA ] = -1;
	instance[ i_boneIndexInputB ] = -1;
	instance[ i_boneIndexOutput ] = -1;
	instance[ i_controlValue ] = 1.f;

	CacheBoneIndex( instance );
}

void CBehaviorGraphConstraintNodeBoneInterpolate::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_controlValue );
	INST_PROP( i_boneIndexInputA );
	INST_PROP( i_boneIndexInputB );
	INST_PROP( i_boneIndexOutput );
}

void CBehaviorGraphConstraintNodeBoneInterpolate::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	// Update control value
	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Update( context, instance, timeDelta );

		Float& weight = instance[ i_controlValue ];
		weight = m_cachedControlValueNode->GetValue( instance );
		weight = Clamp( weight, 0.f, 1.f );
	}
}

void CBehaviorGraphConstraintNodeBoneInterpolate::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	TBaseClass::Sample( context, instance, output );
	
	const Float weight = instance[ i_controlValue ];

	ASSERT( weight >= 0.f && weight <= 1.f );

	const Int32 bIndA = instance[ i_boneIndexInputA ];
	const Int32 bIndB = instance[ i_boneIndexInputB ];
	const Int32 bIndO = instance[ i_boneIndexOutput ];

	if ( bIndA != -1 && bIndB != -1 && bIndO != -1  )
	{
		ASSERT( (Int32)output.m_numBones > bIndA );
		ASSERT( (Int32)output.m_numBones > bIndB );
		ASSERT( (Int32)output.m_numBones > bIndO );

		const AnimQsTransform& transA = output.m_outputPose[ bIndA ];
		const AnimQsTransform& transB = output.m_outputPose[ bIndB ];
		AnimQsTransform& transOut = output.m_outputPose[ bIndO ];

		transOut.Lerp( transA, transB, weight );
	}
}

void CBehaviorGraphConstraintNodeBoneInterpolate::CacheBoneIndex( CBehaviorGraphInstance& instance ) const
{
	instance[ i_boneIndexInputA ] = FindBoneIndex( m_boneInputA, instance );
	instance[ i_boneIndexInputB ] = FindBoneIndex( m_boneInputB, instance );
	instance[ i_boneIndexOutput ] = FindBoneIndex( m_boneOutput, instance );
}