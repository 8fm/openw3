/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphRotateBone.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "animatedComponent.h"
#include "animMath.h"
#include "skeleton.h"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphRotateBoneNode );
IMPLEMENT_RTTI_ENUM( EBoneRotationAxis );

CBehaviorGraphRotateBoneNode::CBehaviorGraphRotateBoneNode()
	: m_axis( ROTAXIS_X )
	, m_scale( 1.0f )
	, m_biasAngle( 0.0f )
	, m_minAngle( -90.0f )
	, m_maxAngle( 90.0f )
	, m_clampRotation( false )
	, m_localSpace( true )
{
}

void CBehaviorGraphRotateBoneNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_boneIndex;
	compiler << i_currentAngle;
}

void CBehaviorGraphRotateBoneNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_boneIndex ] = FindBoneIndex( m_boneName, instance );
	instance[ i_currentAngle ] = 0.f;
}

void CBehaviorGraphRotateBoneNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_boneIndex );
	INST_PROP( i_currentAngle );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphRotateBoneNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Angle ) ) );

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( MinAngle ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( MaxAngle ) ) );
}

void CBehaviorGraphRotateBoneNode::OnPropertyPostChange( IProperty* property )
{
	// Pass to base class
	TBaseClass::OnPropertyPostChange( property );

// 	// When bone name is changed, then cache bone index :)
// 	if ( property->GetName() == CNAME( boneName ) )
// 	{
// 		CacheBoneIndex();
// 	}
}

#endif

void CBehaviorGraphRotateBoneNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( RotBone );

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

void CBehaviorGraphRotateBoneNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( RotBone );

	ANIM_NODE_PRE_SAMPLE

	TBaseClass::Sample( context, instance, output );

	const Float currentAngle = instance[ i_currentAngle ];
	const Int32 boneIndex = instance[ i_boneIndex ];

	// Apply local transformation
	if ( boneIndex != -1 && currentAngle != 0.0f )
	{
		if ( m_localSpace )
		{
#ifdef USE_HAVOK_ANIMATION
			// Assemble rotation
			hkVector4 axes[ 3 ] = { hkVector4( 1, 0, 0, 1 ), hkVector4( 0, 1, 0, 1 ), hkVector4( 0, 0, 1, 1 ) };
			hkQuaternion rotationQuat( axes[ m_axis ], DEG2RAD( currentAngle ) );

			// Assemble transform
			hkQsTransform rotation( hkVector4(0.0f, 0.0f, 0.0f, 0.0f ), rotationQuat, hkVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

			// Apply transform
			ASSERT( boneIndex < (Int32)output.m_numBones );
			output.m_outputPose[ boneIndex ].setMul( output.m_outputPose[ boneIndex ], rotation );
#else
			// Assemble rotation
			RedVector4 axes[ 3 ] = { RedVector4( 1, 0, 0, 1 ), RedVector4( 0, 1, 0, 1 ), RedVector4( 0, 0, 1, 1 ) };
			RedQuaternion rotationQuat( axes[ m_axis ], DEG2RAD( currentAngle ) );

			// Assemble transform
			RedQsTransform rotation( RedVector4(0.0f, 0.0f, 0.0f, 0.0f ), rotationQuat, RedVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

			// Apply transform
			ASSERT( boneIndex < (Int32)output.m_numBones );
			output.m_outputPose[ boneIndex ].SetMul( output.m_outputPose[ boneIndex ], rotation );
#endif
		}
		else
		{
			const CAnimatedComponent *animComponent = instance.GetAnimatedComponent();
			const Int32 parentBoneIndex = animComponent->GetSkeleton()->GetParentBoneIndex(boneIndex);

			AnimQsTransform  parentBoneTransform = output.GetBoneModelTransform( animComponent, parentBoneIndex );

#ifdef USE_HAVOK_ANIMATION
			// find inverse-srerse
			hkQsTransform parentTransformInv;
			parentTransformInv.setInverse( parentBoneTransform );

			// find current model transform of the bone
			hkQsTransform boneWorldTransform;
			boneWorldTransform.setMul( parentBoneTransform, output.m_outputPose[ boneIndex ] );

			// apply rotation
			hkQsTransform rotationRequested;
			rotationRequested.setIdentity();

			hkVector4 axes[ 3 ] = { hkVector4( 1, 0, 0, 1 ), hkVector4( 0, 1, 0, 1 ), hkVector4( 0, 0, 1, 1 ) };			
			rotationRequested.m_rotation = hkQuaternion( axes[ m_axis ], DEG2RAD( currentAngle ) );
			rotationRequested.setMul( rotationRequested, boneWorldTransform );

			// set final rotation
			output.m_outputPose[ boneIndex ].setMul( parentTransformInv, rotationRequested );
#else
			// find inverse-srerse
			RedQsTransform parentTransformInv;
			parentTransformInv.SetInverse( parentBoneTransform );

			// find current model transform of the bone
			RedQsTransform boneWorldTransform;
			boneWorldTransform.SetMul( parentBoneTransform, output.m_outputPose[ boneIndex ] );

			// apply rotation
			RedQsTransform rotationRequested;
			rotationRequested.SetIdentity();

			RedVector4 axes[ 3 ] = { RedVector4( 1, 0, 0, 1 ), RedVector4( 0, 1, 0, 1 ), RedVector4( 0, 0, 1, 1 ) };			
			rotationRequested.Rotation = RedQuaternion( axes[ m_axis ], DEG2RAD( currentAngle ) );
			rotationRequested.SetMul( rotationRequested, boneWorldTransform );

			// set final rotation
			output.m_outputPose[ boneIndex ].SetMul( parentTransformInv, rotationRequested );
#endif
		}
	}

	ANIM_NODE_POST_SAMPLE
}

void CBehaviorGraphRotateBoneNode::OnActivated( CBehaviorGraphInstance& instance ) const
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

void CBehaviorGraphRotateBoneNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
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

void CBehaviorGraphRotateBoneNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedControlVariableNode = CacheValueBlock( TXT("Angle") );
	m_cachedAngleMinNode = CacheValueBlock( TXT("MinAngle") );
	m_cachedAngleMaxNode = CacheValueBlock( TXT("MaxAngle") );
}

void CBehaviorGraphRotateBoneNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
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

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphRotateLimitNode );

CBehaviorGraphRotateLimitNode::CBehaviorGraphRotateLimitNode()
	: m_axis( A_X )
	, m_minAngle( -180.0f )
	, m_maxAngle( 180.0f )
{
}

void CBehaviorGraphRotateLimitNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
	compiler << i_boneIndex;
}

void CBehaviorGraphRotateLimitNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );
	instance[ i_boneIndex ] = FindBoneIndex( m_boneName, instance );
}

void CBehaviorGraphRotateLimitNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );
	INST_PROP_INIT;
	INST_PROP( i_boneIndex );
}

void CBehaviorGraphRotateLimitNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	const Int32 boneIndex = instance[ i_boneIndex ];

	if ( boneIndex != -1 )
	{
#ifdef USE_HAVOK_ANIMATION
		const hkVector4 hkAxis = BehaviorUtils::hkVectorFromAxis( m_axis );
		hkQsTransform& bone = output.m_outputPose[ boneIndex ];

		Float angle;
		hkQuaternion restOut;

		bone.getRotation().decomposeRestAxis( hkAxis, restOut, angle );

		if ( angle > DEG2RAD( m_maxAngle ) )
		{
			angle = DEG2RAD( m_maxAngle );

			hkQuaternion rot;
			rot.setAxisAngle( hkAxis, angle );

			bone.m_rotation.setMul( rot, restOut );
		}
		else if ( angle < DEG2RAD( m_minAngle ) )
		{
			angle = DEG2RAD( m_minAngle );

			hkQuaternion rot;
			rot.setAxisAngle( hkAxis, angle );

			bone.m_rotation.setMul( rot, restOut );
		}
#else
		const RedVector4 hkAxis = BehaviorUtils::RedVectorFromAxis( m_axis );
		RedQsTransform& bone = output.m_outputPose[ boneIndex ];

		Float angle;
		RedQuaternion restOut;

		bone.GetRotation().DecomposeRestAxis( hkAxis, restOut, angle );

		if ( angle > DEG2RAD( m_maxAngle ) )
		{
			angle = DEG2RAD( m_maxAngle );

			RedQuaternion rot;
			rot.SetAxisAngle( hkAxis, angle );

			bone.Rotation.SetMul( rot, restOut );
		}
		else if ( angle < DEG2RAD( m_minAngle ) )
		{
			angle = DEG2RAD( m_minAngle );

			RedQuaternion rot;
			rot.SetAxisAngle( hkAxis, angle );

			bone.Rotation.SetMul( rot, restOut );
		}
#endif
	}
}

//////////////////////////////////////////////////////////////////////////


IMPLEMENT_ENGINE_CLASS( CBehaviorGraphScaleBoneNode )


void CBehaviorGraphScaleBoneNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_boneIndex;
	compiler << i_currentScale;
}

void CBehaviorGraphScaleBoneNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_boneIndex ] = FindBoneIndex( m_boneName, instance );
	instance[ i_currentScale ] = Vector::ONES;
}

void CBehaviorGraphScaleBoneNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_boneIndex );
	INST_PROP( i_currentScale );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphScaleBoneNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( Scale ) ) );
}

#endif

void CBehaviorGraphScaleBoneNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( ScaleBone );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	Vector& currentScale = instance[ i_currentScale ];

	if ( m_cachedControlVariableNode )
	{		
		m_cachedControlVariableNode->Update( context, instance, timeDelta );
		currentScale = m_cachedControlVariableNode->GetVectorValue( instance );
	}
	else
	{
		currentScale = m_scale;
	}
}

void CBehaviorGraphScaleBoneNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( ScaleBone );

	ANIM_NODE_PRE_SAMPLE

	TBaseClass::Sample( context, instance, output );

	const Vector currentScale = instance[ i_currentScale ];
	const Int32 boneIndex = instance[ i_boneIndex ];

	// Apply local transformation
	if ( boneIndex != -1 && currentScale.DistanceSquaredTo( Vector::ONES ) > 0.0001f )
	{
		RedVector4 scaleVec = reinterpret_cast< const RedVector4& > ( currentScale );
		scaleVec.W = 1.f;

		// Assemble transform
		RedQsTransform scale( RedVector4::ZEROS, RedQuaternion::IDENTITY, scaleVec );

		// Apply transform
		ASSERT( boneIndex < (Int32)output.m_numBones );
		output.m_outputPose[ boneIndex ].SetMul( output.m_outputPose[ boneIndex ], scale );
	}

	ANIM_NODE_POST_SAMPLE
}

void CBehaviorGraphScaleBoneNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Activate( instance );
	}
}

void CBehaviorGraphScaleBoneNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Deactivate( instance );
	}
}

void CBehaviorGraphScaleBoneNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedControlVariableNode = CacheVectorValueBlock( TXT("Scale") );
}

void CBehaviorGraphScaleBoneNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->ProcessActivationAlpha( instance, alpha );
	}
}

//////////////////////////////////////////////////////////////////////////