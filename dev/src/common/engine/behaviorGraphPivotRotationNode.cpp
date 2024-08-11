/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphPivotRotationNode.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/animatedComponent.h"
#include "../engine/skeleton.h"
#include "../core/instanceDataLayoutCompiler.h"


IMPLEMENT_ENGINE_CLASS( CBehaviorGraphPivotRotationNode );

CBehaviorGraphPivotRotationNode::CBehaviorGraphPivotRotationNode()
	: m_axis( A_X )
	, m_scale( 1.0f )
	, m_biasAngle( 0.0f )
	, m_minAngle( -90.0f )
	, m_maxAngle( 90.0f )
	, m_clampRotation( false )
{
}

void CBehaviorGraphPivotRotationNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_boneIndex;
	compiler << i_pivotBoneIndex;
	compiler << i_currentAngle;
}

void CBehaviorGraphPivotRotationNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_boneIndex ] = FindBoneIndex( m_boneName, instance );
	instance[ i_pivotBoneIndex ] = FindBoneIndex( m_pivotBoneName, instance );
	instance[ i_currentAngle ] = 0.f;
}

void CBehaviorGraphPivotRotationNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_boneIndex );
	INST_PROP( i_pivotBoneIndex );
	INST_PROP( i_currentAngle );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphPivotRotationNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Angle ) ) );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( MinAngle ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( MaxAngle ) ) );
}

#endif

void CBehaviorGraphPivotRotationNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	Float& currentAngle = instance[ i_currentAngle ];
	Float minAngle, maxAngle;

	if ( m_cachedAngleMinNode )
	{
		m_cachedAngleMinNode->Update( context, instance, timeDelta );
		minAngle = m_cachedAngleMinNode->GetValue( instance );
	}
	else
	{
		minAngle = m_minAngle;
	}

	if ( m_cachedAngleMaxNode )
	{
		m_cachedAngleMaxNode->Update( context, instance, timeDelta );
		maxAngle = m_cachedAngleMaxNode->GetValue( instance );
	}
	else
	{
		maxAngle = m_maxAngle;
	}

	// Update variable
	if ( m_cachedControlVariableNode )
	{
		// Let it compute
		m_cachedControlVariableNode->Update( context, instance, timeDelta );

		// Grab and process value
		currentAngle = m_cachedControlVariableNode->GetValue( instance ) * m_scale + m_biasAngle;
	}
	else
	{
		// Use default value
		currentAngle = m_biasAngle;
	}

	// Clamp if needed
	if ( m_clampRotation )
	{
		currentAngle = Clamp( currentAngle, minAngle, maxAngle );
	}
}

void CBehaviorGraphPivotRotationNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	const Float currentAngle = instance[ i_currentAngle ];
	const Int32 boneIndex = instance[ i_boneIndex ];
	const Int32 pivotBoneIndex = instance[ i_pivotBoneIndex ];

	// Apply local transformation
	if ( boneIndex != -1 && pivotBoneIndex != -1 && currentAngle != 0.0f )
	{
		// Rotation
#ifdef USE_HAVOK_ANIMATION
		hkQuaternion rotationQuat( BehaviorUtils::hkVectorFromAxis( m_axis ), DEG2RAD( currentAngle ) );
		hkQsTransform rotation( hkVector4( 0.f, 0.f, 0.f ), rotationQuat, hkVector4( 1.f, 1.f, 1.f ) );

		// Pivot
		const hkQsTransform& pivotTrans = output.m_outputPose[ pivotBoneIndex ];

		// Rotate pivot
		hkQsTransform rotPivot; rotPivot.setMul( pivotTrans, rotation );

		// Bone new MS
		hkQsTransform boneMS = GetBoneMS( boneIndex, rotPivot, output, instance.GetAnimatedComponent() );

		// Back to LS
		output.m_outputPose[ boneIndex ].setMulInverseMul( pivotTrans, boneMS );
#else
		RedQuaternion rotationQuat( BehaviorUtils::RedVectorFromAxis( m_axis ), DEG2RAD( currentAngle ) );
		RedQsTransform rotation( RedVector4( 0.0f ), rotationQuat, RedVector4( 1.0f ) );

		// Pivot
		const RedQsTransform& pivotTrans = output.m_outputPose[ pivotBoneIndex ];

		// Rotate pivot
		RedQsTransform rotPivot; rotPivot.SetMul( pivotTrans, rotation );

		// Bone new MS
		RedQsTransform boneMS = GetBoneMS( boneIndex, rotPivot, output, instance.GetAnimatedComponent() );

		// Back to LS
		output.m_outputPose[ boneIndex ].SetMulInverseMul( pivotTrans, boneMS );
#endif
	}
}
#ifdef USE_HAVOK_ANIMATION
hkQsTransform CBehaviorGraphPivotRotationNode::GetBoneMS( Int32 boneIndex, const hkQsTransform& parent, const SBehaviorGraphOutput &pose, const CAnimatedComponent* component ) const
{
	// Bone in local space
	hkQsTransform bone = pose.m_outputPose[ boneIndex ];
	// Parent
	bone.setMul( parent, bone );

#else
RedQsTransform CBehaviorGraphPivotRotationNode::GetBoneMS( Int32 boneIndex, const RedQsTransform& parent, const SBehaviorGraphOutput &pose, const CAnimatedComponent* component ) const
{
	// Bone in local space
	RedQsTransform bone = pose.m_outputPose[ boneIndex ];
	// Parent
	bone.SetMul( parent, bone );

#endif
	
	// Parent indices
	const Int16* parentIndices = component->GetSkeleton() ? component->GetSkeleton()->GetParentIndices() : NULL;

	if ( parentIndices )
	{
		// Skip parent
		Int32 parentBone = parentIndices[ boneIndex ];
		if ( parentBone == -1 )
		{
			return bone;
		}

		Int32 currBone = parentIndices[ parentBone ];

		while( currBone != -1 )
		{	
#ifdef USE_HAVOK_ANIMATION
			bone.setMul( pose.m_outputPose[ currBone ], bone );
#else
			bone.SetMul( pose.m_outputPose[ currBone ], bone );
#endif
			currBone = parentIndices[ currBone ];
		}

		return bone;	
	}
	else
	{
		ASSERT( parentIndices );
#ifdef USE_HAVOK_ANIMATION
		return hkQsTransform( hkQsTransform::IDENTITY );
#else
		return RedQsTransform( RedQsTransform::IDENTITY );
#endif
	}
}

void CBehaviorGraphPivotRotationNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Activate( instance );
	}

	if ( m_cachedAngleMinNode )
	{
		m_cachedAngleMinNode->Activate( instance );
	}

	if ( m_cachedAngleMaxNode )
	{
		m_cachedAngleMaxNode->Activate( instance );
	}
}

void CBehaviorGraphPivotRotationNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Deactivate( instance );
	}

	if ( m_cachedAngleMinNode )
	{
		m_cachedAngleMinNode->Deactivate( instance );
	}

	if ( m_cachedAngleMaxNode )
	{
		m_cachedAngleMaxNode->Deactivate( instance );
	}
}

void CBehaviorGraphPivotRotationNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedControlVariableNode = CacheValueBlock( TXT("Angle") );
	m_cachedAngleMinNode = CacheValueBlock( TXT("MinAngle") );
	m_cachedAngleMaxNode = CacheValueBlock( TXT("MaxAngle") );
}

void CBehaviorGraphPivotRotationNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedAngleMinNode )
	{
		m_cachedAngleMinNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedAngleMaxNode )
	{
		m_cachedAngleMaxNode->ProcessActivationAlpha( instance, alpha );
	}
}
