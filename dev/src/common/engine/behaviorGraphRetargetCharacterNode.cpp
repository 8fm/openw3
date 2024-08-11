
#include "build.h"
#include "behaviorGraphRetargetCharacterNode.h"

#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphTranslateBone.h"
#include "behaviorGraphValueNode.h"
#include "../engine/graphConnectionRebuilder.h"
#include "cacheBehaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "animatedComponent.h"
#include "skeleton.h"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphRetargetCharacterNode );
IMPLEMENT_ENGINE_CLASS( IBehaviorGraphRetargetCharacterNodeMethod );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphRetargetCharacterNodeMethod_Scale );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphRetargetCharacterNodeMethod_Skeleton );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphRetargetCharacterNodeMethod_SkeletonMapper );

CBehaviorGraphRetargetCharacterNode::CBehaviorGraphRetargetCharacterNode()
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphRetargetCharacterNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	TBaseClass::OnRebuildSockets();

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
}
#endif

void CBehaviorGraphRetargetCharacterNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_weight;

	if ( m_method )
	{
		m_method->BuildDataLayout( compiler );
	}
}

void CBehaviorGraphRetargetCharacterNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{	
	TBaseClass::OnInitInstance( instance );

	instance[ i_weight ] = 1.f;

	if ( m_method )
	{
		m_method->InitInstance( instance );
	}
}

void CBehaviorGraphRetargetCharacterNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_weight );
}

void CBehaviorGraphRetargetCharacterNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( RetargetCharacter );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Update( context, instance, timeDelta );

		instance[ i_weight ] = m_cachedValueNode->GetValue( instance );
	}

	if ( m_method )
	{
		m_method->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphRetargetCharacterNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( Scale );

	TBaseClass::Sample( context, instance, output );

	const Float weight = instance[ i_weight ];

	if ( m_method )
	{
		m_method->Sample( context, instance, output, weight );
	}
}

void CBehaviorGraphRetargetCharacterNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Activate( instance );
	}
}

void CBehaviorGraphRetargetCharacterNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->Deactivate( instance );
	}
}

void CBehaviorGraphRetargetCharacterNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedValueNode = CacheValueBlock( TXT("Weight") );
}

CSkeleton* CBehaviorGraphRetargetCharacterNode::GetBonesSkeleton( CAnimatedComponent* component ) const
{ 
	return ( m_method != 0 ? m_method->GetBonesSkeleton( component ) : NULL ); 
}

TDynArray<SBehaviorGraphBoneInfo>* CBehaviorGraphRetargetCharacterNode::GetBonesProperty()
{
	return ( m_method != 0 ? m_method->GetBonesProperty() : NULL ); 
}

void CBehaviorGraphRetargetCharacterNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedValueNode )
	{
		m_cachedValueNode->ProcessActivationAlpha( instance, alpha );
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehaviorGraphRetargetCharacterNodeMethod_Scale::CBehaviorGraphRetargetCharacterNodeMethod_Scale()
	: m_scaleFactor( 1.f )
{

}

void CBehaviorGraphRetargetCharacterNodeMethod_Scale::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput& output, Float weight ) const
{
#ifdef USE_HAVOK_ANIMATION
	const hkSimdRealParameter scale = weight * m_scaleFactor;

	for ( Uint32 i=0; i<output.m_numBones; ++i )
	{
		output.m_outputPose[ i ].m_translation.mul4( scale );
	}

	output.m_deltaReferenceFrameLocal.m_translation.mul4( scale );
#else
	const RedVector4 scale( weight * m_scaleFactor ); // replicate

	for ( Uint32 i=0; i<output.m_numBones; ++i )
	{
		SetMul( output.m_outputPose[ i ].Translation, scale );
	}

	SetMul( output.m_deltaReferenceFrameLocal.Translation, scale);
#endif
}

//////////////////////////////////////////////////////////////////////////

CBehaviorGraphRetargetCharacterNodeMethod_Skeleton::CBehaviorGraphRetargetCharacterNodeMethod_Skeleton()
	: m_pelvisBoneName( CNAME( pelvis ) )
{
}

void CBehaviorGraphRetargetCharacterNodeMethod_Skeleton::BuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_motionScale;
	compiler << i_translationBones;
	compiler << i_scaleBones;
}

void CBehaviorGraphRetargetCharacterNodeMethod_Skeleton::InitInstance( CBehaviorGraphInstance& instance ) const
{
	Float& scale = instance[ i_motionScale ];
	scale = 1.f;

	const CSkeleton* sourceRig = m_skeleton.Get();
	const CSkeleton* targetRig = instance.GetAnimatedComponent()->GetSkeleton();

	const Int32 pelvisBoneIdx = targetRig->FindBoneByName( m_pelvisBoneName );
	if ( pelvisBoneIdx != -1 && sourceRig )
	{
		const AnimQsTransform pelvisA = targetRig->GetBoneMS( pelvisBoneIdx );
		const AnimQsTransform pelvisB = sourceRig->GetBoneMS( pelvisBoneIdx );

		// Hmm is this ok?
#ifdef USE_HAVOK_ANIMATION
		const Float zA = pelvisA.m_translation( 2 );
		const Float zB = pelvisB.m_translation( 2 );
#else
		const Float zA = pelvisA.Translation.Z;
		const Float zB = pelvisB.Translation.Z;
#endif
		scale = zA > 0.f && zB > 0.f ? zA / zB : 1.f;
	}

	// clear old bones
	const Int32 boneNum = m_skeleton? m_skeleton->GetBonesNum() : 0;
	instance[ i_translationBones ].Clear();
	instance[ i_scaleBones ].Clear();
	instance[ i_translationBones ].Reserve( boneNum );
	instance[ i_scaleBones ].Reserve( boneNum );

	// add all bones to the list (except root & pelvis)
	for( Int32 i=pelvisBoneIdx + 1; i<boneNum; ++i )
	{
		// In general, we need bones remapping here:
		instance[ i_translationBones ].PushBack( i );
	}

	// remove from translation bones the 'scale only' bones (pelvis & helpers)
	for( Uint32 i=0; i<m_scaleOnlyBones.Size(); ++i )
	{
		// find scale bone
		Int32 scaleBoneIdx = m_skeleton->FindBoneByName( m_scaleOnlyBones[ i ].m_boneName.AsChar() );
		if( scaleBoneIdx < 0 )
		{
			continue;
		}

		instance[ i_translationBones ].Remove( scaleBoneIdx );
		instance[ i_scaleBones ].PushBack( scaleBoneIdx );
	}

	// add pelvis to scale bones
	instance[ i_scaleBones ].PushBack( pelvisBoneIdx );
}

void CBehaviorGraphRetargetCharacterNodeMethod_Skeleton::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput& output, Float weight ) const
{
	const Float scale = instance[ i_motionScale ];

#ifdef USE_HAVOK_ANIMATION
	if ( m_skeleton )
	{
		const Int32 poseNum = (Int32)output.m_numBones;
		const Int32 skNum = m_skeleton->GetBonesNum();

		const Int32 size = Min( poseNum, skNum );
		
		const hkQsTransform* refPose = m_skeleton->GetReferencePose();

		for ( Int32 i=3; i<size; ++i )
		{
			output.m_outputPose[ i ].m_translation = refPose[ i ].m_translation;
		}

		if ( size > 2 )
		{
			output.m_outputPose[ 2 ].m_translation.mul4( scale );
		}
	}

	output.m_deltaReferenceFrameLocal.m_translation.mul4( scale );
#else
	if ( m_skeleton )
	{
		const Int32 poseNum = (Int32)output.m_numBones;
		const Int32 skNum = m_skeleton->GetBonesNum();

		const RedQsTransform* targetRig =  instance.GetAnimatedComponent()->GetSkeleton()->GetReferencePoseLS();

		const Int32 size = Min( poseNum, skNum );
		const Uint32 numTranslationBones = instance[ i_translationBones ].Size();
		const Uint32 numScaleBones = instance[ i_scaleBones ].Size();

		// translation
		for( Uint32 i=0; i<numTranslationBones; ++i )
		{
			const Int32& boneIdx = instance[ i_translationBones ][ i ];
			if( boneIdx >= size )
				continue;

			output.m_outputPose[ boneIdx ].Translation = targetRig[ boneIdx ].Translation;
		}

		// scale
		for( Uint32 i=0; i<numScaleBones; ++i )
		{
			const Int32& boneIdx = instance[ i_scaleBones ][ i ];
			if( boneIdx >= size )
				continue;

			SetMul( output.m_outputPose[ boneIdx ].Translation, scale );
		}
	}

	SetMul( output.m_deltaReferenceFrameLocal.Translation, scale );
#endif
}

//////////////////////////////////////////////////////////////////////////

void CBehaviorGraphRetargetCharacterNodeMethod_SkeletonMapper::BuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_mapper;
}

void CBehaviorGraphRetargetCharacterNodeMethod_SkeletonMapper::InitInstance( CBehaviorGraphInstance& instance ) const
{
	const CSkeleton* thisSkeleton = instance.GetAnimatedComponent()->GetSkeleton();
	if ( m_skeleton && thisSkeleton != m_skeleton.Get() )
	{
		const CSkeleton2SkeletonMapper* m = thisSkeleton->FindSkeletonMapper( m_skeleton.Get() );
		instance[ i_mapper ] = const_cast< CSkeleton2SkeletonMapper* >( m ); // No const inside TInstaceVar :(
	}
	else
	{
		instance[ i_mapper ] = NULL;
	}
}

void CBehaviorGraphRetargetCharacterNodeMethod_SkeletonMapper::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput& output, Float weight ) const
{
	const CSkeleton2SkeletonMapper* mapper = instance[ i_mapper ];
	if ( mapper )
	{
		CCacheBehaviorGraphOutput cachePose( context );
		SBehaviorGraphOutput* pose = cachePose.GetPose();
		if ( pose )
		{
			*pose = output;
			mapper->MapPoseA2B( *pose, output, weight );
		}
	}
}
