
#include "build.h"
#include "behaviorGraphAnimationAdditiveSlot.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "cacheBehaviorGraphOutput.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "animatedComponent.h"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationAdditiveSlotNode );

CBehaviorGraphAnimationAdditiveSlotNode::CBehaviorGraphAnimationAdditiveSlotNode()
	: m_additiveType( AT_Ref )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphAnimationAdditiveSlotNode::GetCaption() const
{
	return String::Printf( TXT("Animation add slot [ %s ]"), m_name.AsChar() );
}

void CBehaviorGraphAnimationAdditiveSlotNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Base ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Additive ) ) );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );
}

#endif

void CBehaviorGraphAnimationAdditiveSlotNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	m_cachedBaseInputNode = CacheBlock( TXT("Additive") );
	m_cachedBaseAnimInputNode = CacheBlock( TXT("Base") );
	m_cachedForceTimeNode = NULL;
}

void CBehaviorGraphAnimationAdditiveSlotNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( AnimationAdditiveSlot );

	if ( m_cachedBaseAnimInputNode )
	{
		m_cachedBaseAnimInputNode->Update( context, instance, timeDelta );
	}

	TBaseClass::OnUpdate( context, instance, timeDelta );
}

void CBehaviorGraphAnimationAdditiveSlotNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( AnimationAdditiveSlot );

	if ( m_cachedBaseAnimInputNode )
	{
		m_cachedBaseAnimInputNode->Sample( context, instance, output );
	}

	if ( IsSlotActive( instance ) && IsValid( instance ) )
	{
		CCacheBehaviorGraphOutput cachePose( context );
		SBehaviorGraphOutput* pose = cachePose.GetPose();
		if ( pose )
		{
			TBaseClass::Sample( context, instance, *pose );
			
			Float weight = 1.f;
			if ( IsBlending( instance ) )
			{
				weight = GetBlendWeight( instance );
				ASSERT( weight >= 0.f && weight <= 1.f );
			}

			BehaviorUtils::BlendingUtils::BlendAdditive( output, output, *pose, weight, m_additiveType );
		}
	}
}

void CBehaviorGraphAnimationAdditiveSlotNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedBaseAnimInputNode )
	{
		m_cachedBaseAnimInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphAnimationAdditiveSlotNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedBaseAnimInputNode )
	{
		m_cachedBaseAnimInputNode->Activate( instance );
	}
}

void CBehaviorGraphAnimationAdditiveSlotNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedBaseAnimInputNode )
	{
		m_cachedBaseAnimInputNode->Deactivate( instance );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationSelAdditiveSlotNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphAnimationSelAdditiveSlotNode::GetCaption() const
{
	return String::Printf( TXT("Animation sel add slot [ %s ]"), m_name.AsChar() );
}

#endif

void CBehaviorGraphAnimationSelAdditiveSlotNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_firstBoneIndex;
	compiler << i_lastBoneIndex;
}

void CBehaviorGraphAnimationSelAdditiveSlotNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_firstBoneIndex ] = FindBoneIndex( m_firstBone, instance );
	instance[ i_lastBoneIndex ] = FindBoneIndex( m_lastBone, instance );
}

void CBehaviorGraphAnimationSelAdditiveSlotNode::AdditiveBlend( CBehaviorGraphInstance& instance, SBehaviorGraphOutput& output, SBehaviorGraphOutput& pose ) const
{
	const Int32 firstBone = instance[ i_firstBoneIndex ];
	const Int32 lastBone = instance[ i_lastBoneIndex ];

	if ( firstBone != -1 && lastBone != -1 )
	{
		ASSERT( firstBone < (Int32)output.m_numBones );
		ASSERT( firstBone < (Int32)pose.m_numBones );
		ASSERT( lastBone < (Int32)output.m_numBones );
		ASSERT( lastBone < (Int32)pose.m_numBones );

		for( Int32 i=firstBone; i<lastBone; ++i )
		{
#ifdef USE_HAVOK_ANIMATION
			output.m_outputPose[i].setMulEq( pose.m_outputPose[i] );
#else
			output.m_outputPose[i].SetMulEq( pose.m_outputPose[i] );
#endif
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicAdditiveSlotNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphMimicAdditiveSlotNode::GetCaption() const
{
	return String::Printf( TXT("Add mimic slot [ %s ]"), m_name.AsChar() );
}

void CBehaviorGraphMimicAdditiveSlotNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( CNAME( Base ) ) );
	CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( CNAME( Additive ) ) );

	CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );
}

#endif

void CBehaviorGraphMimicAdditiveSlotNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedBaseInputNode = CacheMimicBlock( TXT("Additive") );
	m_cachedBaseAnimInputNode = CacheMimicBlock( TXT("Base") );
	m_cachedForceTimeNode = NULL;
}

Bool CBehaviorGraphMimicAdditiveSlotNode::IsValid( CBehaviorGraphInstance& instance ) const
{
	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	ASSERT( ac );

	if ( !ac->GetMimicSkeleton() )
	{
		return false;
	}

	return true;
}

Bool CBehaviorGraphMimicAdditiveSlotNode::IsSlotPoseMimic() const
{
	return true;
}

void CBehaviorGraphMimicAdditiveSlotNode::SampleSlotAnimation( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CBehaviorGraphMimicsAnimationNode::Sample( context, instance, output );
}
