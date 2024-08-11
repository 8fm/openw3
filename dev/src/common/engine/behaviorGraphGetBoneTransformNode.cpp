/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphGetBoneTransformNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "behaviorGraphContext.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphGetBoneTransformNode );
IMPLEMENT_RTTI_ENUM( ETransformType );

CBehaviorGraphGetBoneTransformNode::CBehaviorGraphGetBoneTransformNode()
{
}

void CBehaviorGraphGetBoneTransformNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_vecValue;
	compiler << i_boneIndex;
}

void CBehaviorGraphGetBoneTransformNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_vecValue ] = Vector::ZERO_3D_POINT;
	instance[ i_boneIndex ] = FindBoneIndex( m_boneName, instance );
}

void CBehaviorGraphGetBoneTransformNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_vecValue );
	INST_PROP( i_boneIndex );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphGetBoneTransformNode::OnPropertyPostChange( IProperty *prop )
{
	if ( prop->GetName() == CNAME( type ) )
	{
		OnRebuildSockets();
	}
}

void CBehaviorGraphGetBoneTransformNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );		
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );	
	if ( m_type == TT_Translation )
		CreateSocket( CBehaviorGraphVectorVariableOutputSocketSpawnInfo( CNAME( Translation ) ) );
	else if ( m_type == TT_Rotation )
		CreateSocket( CBehaviorGraphVectorVariableOutputSocketSpawnInfo( CNAME( Rotation ) ) );
	else if ( m_type == TT_Scale )
		CreateSocket( CBehaviorGraphVectorVariableOutputSocketSpawnInfo( CNAME( Scale ) ) );
}

String CBehaviorGraphGetBoneTransformNode::GetCaption() const
{	
	const Char* name( TXT("transform") );
	if ( m_type == TT_Translation )
		name = TXT("translation");
	else if ( m_type == TT_Rotation )
		name = TXT("rotation");
	else if ( m_type == TT_Scale )
		name = TXT("scale");
	return String::Printf( TXT("Get local %s of %s"), name, m_boneName.AsChar() );
}

#endif

void CBehaviorGraphGetBoneTransformNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphGetBoneTransformNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	const Int32 boneIndex = instance[ i_boneIndex ];

	if ( boneIndex != -1 && m_cachedInputNode )
	{
		m_cachedInputNode->Sample( context, instance, output );

		if ( m_type == TT_Translation )
		{
#ifdef USE_HAVOK_ANIMATION
			instance[ i_vecValue ] = TO_CONST_VECTOR_REF( output.m_outputPose[ boneIndex ].getTranslation() );
#else
			instance[ i_vecValue ] = reinterpret_cast< const Vector& >( output.m_outputPose[ boneIndex ].GetTranslation() );
#endif
		}
		else if ( m_type == TT_Rotation )
		{
#ifdef USE_HAVOK_ANIMATION
			EulerAngles angles;

			hkQuaternion q = output.m_outputPose[ boneIndex ].getRotation();
			q.normalize();
			HavokQuaternionToEulerAngles( q, angles ); 
			instance[ i_vecValue ].X = angles.Roll;
			instance[ i_vecValue ].Y = angles.Pitch;
			instance[ i_vecValue ].Z = angles.Yaw;
#else
			RedEulerAngles angles;
			RedQuaternion q = output.m_outputPose[ boneIndex ].GetRotation();
			q.Normalize();
			RedMatrix4x4 conversionMatrix = BuildFromQuaternion( q.Quat );
			angles = conversionMatrix.ToEulerAnglesFull();
			instance[ i_vecValue ].X = angles.Roll;
			instance[ i_vecValue ].Y = angles.Pitch;
			instance[ i_vecValue ].Z = angles.Yaw;
#endif
		}
		else if ( m_type == TT_Scale )
		{
#ifdef USE_HAVOK_ANIMATION
			instance[ i_vecValue ] = TO_CONST_VECTOR_REF( output.m_outputPose[ boneIndex ].getScale() );
#else
			instance[ i_vecValue ] = reinterpret_cast< const Vector& >( output.m_outputPose[ boneIndex ].GetScale() );
#endif
		}
	}	
}

void CBehaviorGraphGetBoneTransformNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );
	if ( m_type == TT_Scale )
		instance[ i_vecValue ].SetOnes();
	else
		instance[ i_vecValue ].SetZeros();
}

Vector CBehaviorGraphGetBoneTransformNode::GetVectorValue( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_vecValue ];
}

void CBehaviorGraphGetBoneTransformNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphGetBoneTransformNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}
}

void CBehaviorGraphGetBoneTransformNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}
}

void CBehaviorGraphGetBoneTransformNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheBlock( TXT("Input") );
}
