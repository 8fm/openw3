/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorNodeParentChild.h"
#include "behaviorGraphInstance.h"
#include "../engine/skeleton.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "behaviorProfiler.h"
#include "behaviorGraphUtils.inl"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorNodeParentChild );

CBehaviorNodeParentChild::CBehaviorNodeParentChild()
	: m_parentBoneName( TXT( "pelvis" ) )
	, m_childBoneName( TXT( "pelvis" ) )
	, m_offset( 0.f, 0.f, 0.f, 0.f )
	, m_changeOnlyTranslation( false )
{
}

void CBehaviorNodeParentChild::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_parentBoneIndex;
	compiler << i_childBoneIndex;
}

void CBehaviorNodeParentChild::OnInitInstance( CBehaviorGraphInstance& instance ) const
{	
	TBaseClass::OnInitInstance( instance );

	instance[ i_parentBoneIndex ] = FindBoneIndex( m_parentBoneName, instance );
	instance[ i_childBoneIndex ] = FindBoneIndex( m_childBoneName, instance );
}

void CBehaviorNodeParentChild::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_parentBoneIndex );
	INST_PROP( i_childBoneIndex );
}

void CBehaviorNodeParentChild::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( ParentChild );

	TBaseClass::Sample( context, instance, output );

	Int32 parentBoneIdx = instance[ i_parentBoneIndex ];
	Int32 childBoneIdx = instance[ i_childBoneIndex ];
	Int32 parentFromHierarchyBoneIdx = instance.GetAnimatedComponent()->GetSkeleton()->GetParentBoneIndex( childBoneIdx );

	if ( parentBoneIdx == -1 || childBoneIdx == -1 || parentFromHierarchyBoneIdx == -1 )
	{
		return;
	}

	AnimQsTransform parentMS = output.GetBoneModelTransform( instance.GetAnimatedComponent(), parentBoneIdx );
	AnimQsTransform parentFromHierarchyMS = output.GetBoneModelTransform( instance.GetAnimatedComponent(), parentFromHierarchyBoneIdx );

	AnimQsTransform newChildLS;
	newChildLS.SetMulInverseMul( parentFromHierarchyMS, parentMS );
	newChildLS.SetTranslation( Add( newChildLS.GetTranslation(), VECTOR_TO_ANIM_CONST_VECTOR_REF( m_offset ) ) );

	AnimQsTransform& childLS = output.m_outputPose[ childBoneIdx ];
	if ( m_changeOnlyTranslation )
	{
		childLS.SetTranslation( newChildLS.GetTranslation() );
	}
	else
	{	
		childLS	= newChildLS;
	}
}

CSkeleton* CBehaviorNodeParentChild::GetBonesSkeleton( CAnimatedComponent* component ) const
{
	return component->GetSkeleton();
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
