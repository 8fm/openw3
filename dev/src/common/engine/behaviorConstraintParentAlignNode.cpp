/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorConstraintParentAlignNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "behaviorGraphValueNode.h"
#include "../engine/animatedComponent.h"
#include "../engine/skeleton.h"
#include "../engine/entity.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../core/instanceDataLayoutCompiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphConstraintNodeParentAlign );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphConstraintReset );

CBehaviorGraphConstraintNodeParentAlign::CBehaviorGraphConstraintNodeParentAlign()
	: m_localSpace( true )
{
}

void CBehaviorGraphConstraintNodeParentAlign::FindParentAnimatedComponent( CBehaviorGraphInstance& instance ) const
{
	const CAnimatedComponent* animatedComponent = instance.GetAnimatedComponent();

	if ( animatedComponent && animatedComponent->GetTransformParent() )
	{
		const CHardAttachment* attachment = animatedComponent->GetTransformParent();
		if ( attachment->GetParent() )
		{
			const CNode* parent = attachment->GetParent();

			if ( parent->IsA< CAnimatedComponent >() )
			{
				instance[ i_parentAnimatedComponent ] = reinterpret_cast< TGenericPtr >( const_cast< CNode* >( parent ) );
			}
		}
	}

	BEH_WARN( TXT("CBehaviorGraphConstraintNodeParentAlign: Couldn't find parent animated component for '%ls' in '%ls' '%ls'"), 
		GetName().AsChar(), animatedComponent->GetName().AsChar(), animatedComponent->GetEntity()->GetName().AsChar() );
}

const CAnimatedComponent* CBehaviorGraphConstraintNodeParentAlign::GetParentAnimatedComponent( CBehaviorGraphInstance& instance ) const
{
	return reinterpret_cast< const CAnimatedComponent* >( instance[ i_parentAnimatedComponent ] );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphConstraintNodeParentAlign::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );		
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ), false ) );
}

#endif

void CBehaviorGraphConstraintNodeParentAlign::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache input
	m_cachedControlValueNode = CacheValueBlock( TXT("Weight") );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphConstraintNodeParentAlign::GetCaption() const
{
	if ( !m_name.Empty() )
		return String::Printf( TXT("Parent align - %s"), m_name.AsChar() );
	else
		return String( TXT("Parent align") );;
}

#endif

void CBehaviorGraphConstraintNodeParentAlign::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphConstraintNodeParentAlign::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Activate( instance );
	}
}

void CBehaviorGraphConstraintNodeParentAlign::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Deactivate( instance );
	}
}

void CBehaviorGraphConstraintNodeParentAlign::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_parentBoneIndex;
	compiler << i_boneIndex;
	compiler << i_controlValue;
	compiler << i_parentAnimatedComponent;
}

void CBehaviorGraphConstraintNodeParentAlign::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_parentBoneIndex ] = -1;
	instance[ i_boneIndex ] = -1;
	instance[ i_controlValue ] = 1.f;
	
	FindParentAnimatedComponent( instance );

	CacheBoneIndex( instance );
}

void CBehaviorGraphConstraintNodeParentAlign::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_controlValue );
	INST_PROP( i_boneIndex );
	INST_PROP( i_parentBoneIndex );
	INST_PROP( i_parentAnimatedComponent );
}

void CBehaviorGraphConstraintNodeParentAlign::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	// Update control value
	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Update( context, instance, timeDelta );

		Float& weight = instance[ i_controlValue ];
		weight = m_cachedControlValueNode->GetValue( instance );

		// Clamp value
		weight = ( weight > 0.5f ) ? 1.0f : 0.0f;
	}
}

void CBehaviorGraphConstraintNodeParentAlign::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	TBaseClass::Sample( context, instance, output );

	const CAnimatedComponent* animatedComponent = instance.GetAnimatedComponent();
	const CAnimatedComponent* parentAnimatedComponent = GetParentAnimatedComponent( instance );

	const Int32 boneIndex = instance[ i_boneIndex ];
	const Int32 parentBoneIndex = instance[ i_parentBoneIndex ];

	ASSERT( (Int32)output.m_numBones > boneIndex );

	if ( instance[ i_controlValue ] > 0.5f && parentAnimatedComponent && CheckBones( instance ) && 
		animatedComponent && (Int32)output.m_numBones > boneIndex )
	{
		// Copy parent bone transform to child bone

		if ( m_localSpace )
		{
			output.m_outputPose[ boneIndex ] = parentAnimatedComponent->GetBoneTransformLocalSpace( parentBoneIndex );
			/*
			const Int32 boneIndexA = instance[ i_boneIndexA ];
			const Int32 boneIndexB = instance[ i_boneIndexB ];
			const Int32 boneIndexOut = instance[ i_boneIndexOut ];

			const AnimQsTransform& transA = output.m_outputPose[ boneIndexA ];
			const AnimQsTransform& transB = output.m_outputPose[ boneIndexB ];
			AnimQsTransform& transOut = output.m_outputPose[ boneIndexOut ];

			transOut.Lerp( transA, transB, weight );
			*/
		}
		else if ( animatedComponent->GetSkeleton() && animatedComponent->GetSkeleton()->IsValid() )
		{
			CSkeleton* skeleton = animatedComponent->GetSkeleton();
			if ( !skeleton )
			{
				return;
			}
#ifdef USE_HAVOK_ANIMATION
			// Parent MS
			hkQsTransform parentOrgTransMS = output.GetBoneModelTransform( animatedComponent, skeleton->GetParentIndices()[ boneIndex ] );

			hkQsTransform parentOrgTransMSInv;
			parentOrgTransMSInv.setInverse( parentOrgTransMS );

			// Dest bone MS
			hkQsTransform boneDestMS;
			MatrixToHavokQsTransform( parentAnimatedComponent->GetBoneMatrixModelSpace( parentBoneIndex ), boneDestMS );

			// Local space
			output.m_outputPose[ boneIndex ].setMul( parentOrgTransMSInv, boneDestMS );
#else
			// Parent MS
			RedQsTransform parentOrgTransMS = output.GetBoneModelTransform( animatedComponent, skeleton->GetParentIndices()[ boneIndex ] );

			RedQsTransform parentOrgTransMSInv;
			parentOrgTransMSInv.SetInverse( parentOrgTransMS );

			// Dest bone MS
			RedQsTransform boneDestMS;
			Matrix store = parentAnimatedComponent->GetBoneMatrixModelSpace( parentBoneIndex );
			RedMatrix4x4 conversionMatrix = reinterpret_cast< const RedMatrix4x4& >( store );
			boneDestMS.Set( conversionMatrix );

			// Local space
			output.m_outputPose[ boneIndex ].SetMul( parentOrgTransMSInv, boneDestMS );
#endif
		}
	}
}

void CBehaviorGraphConstraintNodeParentAlign::CacheBoneIndex( CBehaviorGraphInstance& instance ) const
{
	const CAnimatedComponent* animatedComponent = instance.GetAnimatedComponent();
	const CAnimatedComponent* parentAnimatedComponent = GetParentAnimatedComponent( instance );

	instance[ i_boneIndex ] = FindBoneIndex( m_bone, animatedComponent );
	instance[ i_parentBoneIndex ] = FindBoneIndex( m_parentBone, parentAnimatedComponent );
}

Bool CBehaviorGraphConstraintNodeParentAlign::CheckBones( CBehaviorGraphInstance& instance ) const
{
	return ( instance[ i_boneIndex ] != -1 && instance[ i_parentBoneIndex ] != -1 ) ? true : false;
}

void CBehaviorGraphConstraintNodeParentAlign::OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const
{
	FindParentAnimatedComponent( instance );
}

//////////////////////////////////////////////////////////////////////////

CBehaviorGraphConstraintReset::CBehaviorGraphConstraintReset()
	: m_translation( true )
	, m_rotation( true )
	, m_scale( true )
{

}

void CBehaviorGraphConstraintReset::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_boneIndex;
	compiler << i_controlValue;
}

void CBehaviorGraphConstraintReset::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_boneIndex ] = FindBoneIndex( m_bone, instance );
	instance[ i_controlValue ] = false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphConstraintReset::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );		
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ), false ) );
}

#endif

void CBehaviorGraphConstraintReset::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache input
	m_cachedControlValueNode = CacheValueBlock( TXT("Weight") );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphConstraintReset::GetCaption() const
{
	if ( !m_name.Empty() )
		return String::Printf( TXT("Reset - %s"), m_name.AsChar() );
	else
		return String( TXT("Reset") );
}

#endif

void CBehaviorGraphConstraintReset::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphConstraintReset::OnReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_controlValue ] = false;
}

void CBehaviorGraphConstraintReset::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Activate( instance );
	}
}

void CBehaviorGraphConstraintReset::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Deactivate( instance );
	}
}

void CBehaviorGraphConstraintReset::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	// Update control value
	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Update( context, instance, timeDelta );
		instance[ i_controlValue ] = ( m_cachedControlValueNode->GetValue( instance ) > 0.5 ) ? true : false;
	}
	else
	{
		instance[ i_controlValue ] = true;
	}
}

void CBehaviorGraphConstraintReset::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	TBaseClass::Sample( context, instance, output );

	const Int32 boneIndex = instance[ i_boneIndex ];

	if ( instance[ i_controlValue ] && boneIndex < (Int32)output.m_numBones )
	{
#ifdef USE_HAVOK_ANIMATION
		if ( m_translation )
		{
			output.m_outputPose[ boneIndex ].m_translation.setZero4();
		}
		if ( m_rotation )
		{
			output.m_outputPose[ boneIndex ].m_rotation.setIdentity();
		}
		if ( m_scale )
		{
			output.m_outputPose[ boneIndex ].m_scale.setAll(1.0f);
		}
#else
		if ( m_translation )
		{
			output.m_outputPose[ boneIndex ].Translation.SetZeros();
		}
		if ( m_rotation )
		{
			output.m_outputPose[ boneIndex ].Rotation.SetIdentity();
		}
		if ( m_scale )
		{
			output.m_outputPose[ boneIndex ].Scale.SetOnes();
		}
#endif
	}
}
