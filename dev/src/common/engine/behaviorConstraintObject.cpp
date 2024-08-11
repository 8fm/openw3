/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorConstraintNode.h"
#include "behaviorConstraintObject.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "../engine/entity.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "animatedComponent.h"
#include "skeleton.h"

IMPLEMENT_ENGINE_CLASS( IBehaviorConstraintObject );
IMPLEMENT_ENGINE_CLASS( CBehaviorConstraintBoneObject );
IMPLEMENT_ENGINE_CLASS( CBehaviorConstraintVectorObject )
IMPLEMENT_ENGINE_CLASS( CBehaviorConstraintComponentObject )

//////////////////////////////////////////////////////////////////////////

IBehaviorConstraintObject::IBehaviorConstraintObject()
	: m_localPositionOffset(0,0,0)
	, m_localRotationOffset(0,0,0)
{

}

void IBehaviorConstraintObject::GetLocalOffsets( CBehaviorGraphInstance& instance, AnimQsTransform& lacalOffset ) const
{
	AnimQuaternion rot;

#ifdef USE_HAVOK_ANIMATION
	EulerAnglesToHavokQuaternion( instance[ i_localRotationOffset ] , rot);

	lacalOffset = hkQsTransform( TO_CONST_HK_VECTOR_REF( instance[ i_localPositionOffset ] ), rot, hkVector4(1.0f,1.0f,1.0f,1.0f));
#else
	Vector quat = instance[ i_localRotationOffset ].ToQuat();
	rot.Quat = reinterpret_cast< const RedVector4& >( quat );
	lacalOffset = RedQsTransform( reinterpret_cast< const RedVector4& >( instance[ i_localPositionOffset ] ), rot, RedVector4::ONES );
#endif
}

CBehaviorGraphConstraintNode* IBehaviorConstraintObject::GetOwner()
{
	CBehaviorGraphConstraintNode* owner = FindParent< CBehaviorGraphConstraintNode >();
	ASSERT( owner );
	return owner;
}

EngineQsTransform IBehaviorConstraintObject::GetTransform( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_objectTransform ];
}

EngineQsTransform IBehaviorConstraintObject::RefreshTransform( CBehaviorGraphInstance& instance ) const
{
	return GetTransform( instance );
}

void IBehaviorConstraintObject::SetTransform( CBehaviorGraphInstance& instance, const AnimQsTransform& transform ) const
{
#ifdef USE_HAVOK_ANIMATION
	EngineQsTransform& engineTrans = instance[ i_objectTransform ];
	HkToEngineQsTransform( transform, engineTrans );
#else
	EngineQsTransform& engineTrans = instance[ i_objectTransform ];
	engineTrans = reinterpret_cast< const EngineQsTransform& >( transform );
#endif
}

void IBehaviorConstraintObject::SetTransformMatrix( CBehaviorGraphInstance& instance, const Matrix& transform ) const
{
	instance[ i_objectTransform ] = transform;
}

void IBehaviorConstraintObject::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_objectTransform;
	compiler << i_localPositionOffset;
	compiler << i_localRotationOffset;
}

void IBehaviorConstraintObject::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	instance[ i_objectTransform ].Identity();
	instance[ i_localPositionOffset ] = m_localPositionOffset;
	instance[ i_localRotationOffset ] = m_localRotationOffset;
}

//////////////////////////////////////////////////////////////////////////

void CBehaviorConstraintBoneObject::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_boneIndex;
}

void CBehaviorConstraintBoneObject::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_boneIndex ] = instance.GetAnimatedComponent()->FindBoneByName( m_boneName.AsChar() );
}

void CBehaviorConstraintBoneObject::Sample( SBehaviorGraphOutput &output, CBehaviorGraphInstance& instance ) const
{
	const Int32 boneIndex = instance[ i_boneIndex ];
#ifdef USE_HAVOK_ANIMATION
	if ( boneIndex != -1 && instance.GetAnimatedComponent()->GetSkeleton() && instance.GetAnimatedComponent()->GetSkeleton()->GetHavokSkeleton() )
	{
		// Get transform in model space
		hkQsTransform modelTrans = output.m_outputPose[ boneIndex ];

		// Get offset
		hkQsTransform localOffset;
		GetLocalOffsets( instance, localOffset );

		// Apply local offset
		modelTrans.setMul( modelTrans, localOffset );

		// Get havok skeleton
		const hkaSkeleton* hkSkeleton = instance.GetAnimatedComponent()->GetSkeleton()->GetHavokSkeleton();

		// Bone transform in model space
		Int32 currBone = hkSkeleton->m_parentIndices[ boneIndex ];
		while( currBone != -1 )
		{			
			modelTrans.setMul( output.m_outputPose[ currBone ], modelTrans );
			currBone = hkSkeleton->m_parentIndices[ currBone ];
		}

		SetTransform( instance, modelTrans );
	}
#else
	if ( boneIndex != -1 && instance.GetAnimatedComponent()->GetSkeleton() )
	{
		// Get transform in model space
		RedQsTransform modelTrans = output.m_outputPose[ boneIndex ];

		// Get offset
		RedQsTransform localOffset;
		GetLocalOffsets( instance, localOffset );

		// Apply local offset
		modelTrans.SetMul( modelTrans, localOffset );

		// Bone transform in model space
		const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetSkeleton();
		Int32 currBone = skeleton->GetParentBoneIndex( boneIndex );
		while( currBone != -1 )
		{			
			modelTrans.SetMul( output.m_outputPose[ currBone ], modelTrans );
			currBone = skeleton->GetParentBoneIndex( currBone );
		}

		SetTransform( instance, modelTrans );
	}
#endif
	else
	{
		SetTransform( instance, AnimQsTransform::IDENTITY );
	}
}

//////////////////////////////////////////////////////////////////////////

void CBehaviorConstraintComponentObject::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	// Reset bone index
	instance[ i_boneIndex ] = -1;

	// Find entity and component
	const CEntity* entity = instance.GetAnimatedComponent()->GetEntity();
	const CComponent* component = entity->FindComponent( m_componentName );

	if ( component && component->GetTransformParent() )
	{
		// Find bone
		CName boneName = component->GetTransformParent()->GetParentSlotName();

		instance[ i_boneIndex ] = instance.GetAnimatedComponent()->FindBoneByName( boneName.AsString().AsChar() );
		instance[ i_localPositionOffset ] = component->GetTransformParent()->GetRelativeTransform().GetPosition();
		instance[ i_localRotationOffset ] = component->GetTransformParent()->GetRelativeTransform().GetRotation();
	}
}

//////////////////////////////////////////////////////////////////////////

CBehaviorConstraintVectorObject::CBehaviorConstraintVectorObject()
{

}

void CBehaviorConstraintVectorObject::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_positionValue;
	compiler << i_rotationValue;
}

void CBehaviorConstraintVectorObject::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_positionValue ] = Vector::ZERO_3D_POINT;
	instance[ i_rotationValue ] = Vector::ZERO_3D_POINT;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorConstraintVectorObject::CreateSocketForOwner( CBehaviorGraphConstraintNode* owner )
{
	owner->CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( TargetPosition ) ) );
	owner->CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( TargetRotation ) ) );
}

#endif

EngineQsTransform CBehaviorConstraintVectorObject::RefreshTransform( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedTargetPositionNode )
	{
		instance[ i_positionValue ] = m_cachedTargetPositionNode->GetVectorValue( instance );
	}
	if ( m_cachedTargetRotationNode )
	{
		instance[ i_rotationValue ] = m_cachedTargetRotationNode->GetVectorValue( instance );
	}

	const Vector& positionValue = instance[ i_positionValue ];
	const Vector& rotationValue = instance[ i_rotationValue ];
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform localOffset;
	GetLocalOffsets( instance, localOffset );

	hkQsTransform objectTransform( hkQsTransform::IDENTITY );
	objectTransform.setTranslation( TO_CONST_HK_VECTOR_REF( positionValue ) );

	hkQuaternion rotation;
	EulerAngles angles( rotationValue.X, rotationValue.Y, rotationValue.Z );
	EulerAnglesToHavokQuaternion( angles, rotation );

	objectTransform.setRotation( rotation );
	objectTransform.setMul( localOffset, objectTransform );

#else
	RedQsTransform localOffset;
	GetLocalOffsets( instance, localOffset );

	RedQsTransform objectTransform( RedQsTransform::IDENTITY );
	objectTransform.SetTranslation( reinterpret_cast< const RedVector4&>( positionValue ) );

	RedQuaternion rotation;
	RedEulerAngles angles( rotationValue.X, rotationValue.Y, rotationValue.Z );
	rotation = angles.ToQuaternion();

	objectTransform.SetRotation( rotation );
	objectTransform.SetMul( localOffset, objectTransform );

#endif

	SetTransform( instance, objectTransform );

	return GetTransform( instance );
}

void CBehaviorConstraintVectorObject::Sample( SBehaviorGraphOutput &output, CBehaviorGraphInstance& instance ) const
{
	/*
	//////////////////////////////////////////////////////////////////////////
	// GCom Hack

	if ( m_cachedTargetPositionNode )
	{
		instance[ i_positionValue ] = m_cachedTargetPositionNode->GetVectorValue( instance );
	}
	if ( m_cachedTargetRotationNode )
	{
		instance[ i_rotationValue ] = m_cachedTargetRotationNode->GetVectorValue( instance );
	}

	//////////////////////////////////////////////////////////////////////////
	*/

	const Vector& positionValue = instance[ i_positionValue ];
	const Vector& rotationValue = instance[ i_rotationValue ];
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform localOffset;
	GetLocalOffsets( instance, localOffset );

	hkQsTransform objectTransform( hkQsTransform::IDENTITY );
	objectTransform.setTranslation( TO_CONST_HK_VECTOR_REF( positionValue ) );
		
	hkQuaternion rotation;
	EulerAngles angles( rotationValue.X, rotationValue.Y, rotationValue.Z );
	EulerAnglesToHavokQuaternion( angles, rotation );
	//if ( !m_rotationInLocalSpace ) 
	//{
		objectTransform.setRotation( rotation );
	//}

	objectTransform.setMul( localOffset, objectTransform );
	/*if ( m_rotationInLocalSpace )
	{
		Int32 boneIndex = instance.GetAnimatedComponent()->GetTrajectoryBone();

		if ( boneIndex != -1 )
		{
			hkQsTransform traj = output.m_outputPose[boneIndex];
			traj.m_rotation.normalize();

			Int32 rootBoneIndex = m_owner->GetAnimComponent()->GetSkeleton()->GetHavokSkeleton()->m_parentIndices[boneIndex];
			hkQsTransform root = output.m_outputPose[rootBoneIndex];
			root.m_rotation.normalize();

			hkQsTransform trajModel;
			trajModel.setMul(root, traj);

			hkQsTransform rotationTransform(hkVector4(0.0f,0.0f,0.0f,0.0f), rotation, hkVector4(1.0f,1.0f,1.0f,1.0f));
			rotationTransform.setInterpolate4( rotationTransform, hkQsTransform::getIdentity(), m_owner->GetControlValue() );

			rotationTransform.setMulInverseMul(rotationTransform, trajModel);
			//rotationTransform.setMulMulInverse(trajModel, rotationTransform);

			m_objectTransform.setMul(rotationTransform, m_objectTransform);
		}
		else
		{
			//const hkQsTransform root = output.m_outputPose[0];
			//root.m_rotation.normalize();

			//hkQsTransform rotationTransform(hkVector4(0.0f,0.0f,0.0f,0.0f), rotation, hkVector4(1.0f,1.0f,1.0f,1.0f));
			//rotationTransform.setInterpolate4( rotationTransform, hkQsTransform::getIdentity(), m_owner->GetControlValue() );

			//rotationTransform.setMulInverseMul( rotationTransform, root );

			//m_objectTransform.setMul( rotationTransform, objectTransform );
		}
	}*/

	// Apply motion extraction
	if ( instance.GetAnimatedComponent()->UseExtractedMotion() )
	{
		objectTransform.setMulInverseMul( output.m_deltaReferenceFrameLocal, objectTransform );
	}
#else
	RedQsTransform localOffset;
	GetLocalOffsets( instance, localOffset );

	RedQsTransform objectTransform( RedQsTransform::IDENTITY );
	objectTransform.SetTranslation( reinterpret_cast< const RedVector4& >( positionValue ) );
		
	RedQuaternion rotation;
	RedEulerAngles angles( rotationValue.X, rotationValue.Y, rotationValue.Z );
	rotation = angles.ToQuaternion();
	//EulerAnglesToHavokQuaternion( angles, rotation );
	//if ( !m_rotationInLocalSpace ) 
	//{
		objectTransform.SetRotation( rotation );
	//}

	objectTransform.SetMul( localOffset, objectTransform );
	/*if ( m_rotationInLocalSpace )
	{
		Int32 boneIndex = instance.GetAnimatedComponent()->GetTrajectoryBone();

		if ( boneIndex != -1 )
		{
			hkQsTransform traj = output.m_outputPose[boneIndex];
			traj.m_rotation.normalize();

			Int32 rootBoneIndex = m_owner->GetAnimComponent()->GetSkeleton()->GetHavokSkeleton()->m_parentIndices[boneIndex];
			hkQsTransform root = output.m_outputPose[rootBoneIndex];
			root.m_rotation.normalize();

			hkQsTransform trajModel;
			trajModel.setMul(root, traj);

			hkQsTransform rotationTransform(hkVector4(0.0f,0.0f,0.0f,0.0f), rotation, hkVector4(1.0f,1.0f,1.0f,1.0f));
			rotationTransform.setInterpolate4( rotationTransform, hkQsTransform::getIdentity(), m_owner->GetControlValue() );

			rotationTransform.setMulInverseMul(rotationTransform, trajModel);
			//rotationTransform.setMulMulInverse(trajModel, rotationTransform);

			m_objectTransform.setMul(rotationTransform, m_objectTransform);
		}
		else
		{
			//const hkQsTransform root = output.m_outputPose[0];
			//root.m_rotation.normalize();

			//hkQsTransform rotationTransform(hkVector4(0.0f,0.0f,0.0f,0.0f), rotation, hkVector4(1.0f,1.0f,1.0f,1.0f));
			//rotationTransform.setInterpolate4( rotationTransform, hkQsTransform::getIdentity(), m_owner->GetControlValue() );

			//rotationTransform.setMulInverseMul( rotationTransform, root );

			//m_objectTransform.setMul( rotationTransform, objectTransform );
		}
	}*/

	// Apply motion extraction
	if ( instance.GetAnimatedComponent()->UseExtractedMotion() )
	{
		objectTransform.SetMulInverseMul( output.m_deltaReferenceFrameLocal, objectTransform );
	}
#endif



	// Set final transform
	SetTransform( instance, objectTransform );
}

void CBehaviorConstraintVectorObject::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedTargetPositionNode )
	{
		ASSERT( m_cachedTargetPositionNode->IsActive( instance ) );
		m_cachedTargetPositionNode->Update( context, instance, timeDelta );
		instance[ i_positionValue ] = m_cachedTargetPositionNode->GetVectorValue( instance );
	}
	else
	{
		instance[ i_positionValue ] = Vector::ZERO_3D_POINT;
	}

	if ( m_cachedTargetRotationNode )
	{
		ASSERT( m_cachedTargetRotationNode->IsActive( instance ) );
		m_cachedTargetRotationNode->Update( context, instance, timeDelta );
		instance[ i_rotationValue ] = m_cachedTargetRotationNode->GetVectorValue( instance );
	}
	else
	{
		instance[ i_rotationValue ] = Vector::ZERO_3D_POINT;
	}
}

void CBehaviorConstraintVectorObject::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedTargetPositionNode )
	{
		m_cachedTargetPositionNode->Activate( instance );
	}

	if ( m_cachedTargetRotationNode )
	{
		m_cachedTargetRotationNode->Activate( instance );
	}
}

void CBehaviorConstraintVectorObject::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedTargetPositionNode )
	{
		m_cachedTargetPositionNode->Deactivate( instance );
	}

	if ( m_cachedTargetRotationNode )
	{
		m_cachedTargetRotationNode->Deactivate( instance );
	}
}

void CBehaviorConstraintVectorObject::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedTargetPositionNode )
	{
		m_cachedTargetPositionNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedTargetRotationNode )
	{
		m_cachedTargetRotationNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorConstraintVectorObject::CacheConnections( CBehaviorGraphConstraintNode* owner )
{
	// Pass to base class
	TBaseClass::CacheConnections( owner );

	// Cache connections
	m_cachedTargetPositionNode = owner->CacheVectorValueBlock( TXT("TargetPosition") );
	m_cachedTargetRotationNode = owner->CacheVectorValueBlock( TXT("TargetRotation") );
}
