/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "behaviorGraphRecentlyUsedAnimsStateNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "allocatedBehaviorGraphOutput.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "animatedComponent.h"
#include "behaviorProfiler.h"
#include "behaviorGraphContext.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphRecentlyUsedAnimsStateNode );

//////////////////////////////////////////////////////////////////////////

CBehaviorGraphRecentlyUsedAnimsStateNode::CBehaviorGraphRecentlyUsedAnimsStateNode()
	:	m_poseBlendOutTime( 0.2f )
	,	m_applyMotion( true )
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphRecentlyUsedAnimsStateNode::GetCaption() const
{
	String caption = TXT("Recently used anims");
	if ( ! GetName().Empty() )
	{
		caption += String::Printf( TXT(" (%s)"), GetName().AsChar() );
	}
	String inSync = m_behaviorGraphSyncInfo.GetInListAsString();
	if ( ! inSync.Empty() )
	{
		caption += TXT(" : ") + inSync;
	}
	if ( m_behaviorGraphSyncInfo.m_inSyncPriority != 0 )
	{
		caption += String::Printf( TXT(" priority %i"), m_behaviorGraphSyncInfo.m_inSyncPriority );
	}
	return caption;
}

Color CBehaviorGraphRecentlyUsedAnimsStateNode::GetTitleColor() const
{
	return Color( 143, 170, 190 );
}

Bool CBehaviorGraphRecentlyUsedAnimsStateNode::CanBeExpanded() const
{
	return false;
}

#endif

void CBehaviorGraphRecentlyUsedAnimsStateNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_pose;
	compiler << i_poseWeight;
	compiler << i_playbacks;
}

void CBehaviorGraphRecentlyUsedAnimsStateNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_pose );
	INST_PROP( i_poseWeight );
	INST_PROP( i_playbacks );
}

void CBehaviorGraphRecentlyUsedAnimsStateNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );
}

void CBehaviorGraphRecentlyUsedAnimsStateNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReleaseInstance( instance );

	DestroyPose( instance );
}

void CBehaviorGraphRecentlyUsedAnimsStateNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( RecentlyUsedAnimsState );
	TBaseClass::OnUpdate( context, instance, timeDelta );

	Float& poseWeight = instance[ i_poseWeight ];
	const SBehaviorUsedAnimations& recentlyUsedAnims = instance.GetAnimatedComponent()->GetRecentlyUsedAnims();
	if ( recentlyUsedAnims.m_anims.GetNum() == 0 )
	{
		poseWeight = 1.f;
	}
	else if ( m_poseBlendOutTime > 0.f )
	{
		poseWeight = Max( 0.0f, poseWeight - timeDelta / m_poseBlendOutTime );
	}
	else if ( m_poseBlendOutTime < 0.f )
	{
		poseWeight = 1.f;
	}
	else
	{
		poseWeight = 0.f;
	}
	ASSERT( poseWeight >= 0.f && poseWeight <= 1.f );

	instance[ i_playbacks ].UpdatePlayback( timeDelta );
}


void CBehaviorGraphRecentlyUsedAnimsStateNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	const Float& poseWeight = instance[ i_poseWeight ];

	if ( poseWeight < 0.001f )
	{
		// Only animations
		instance[ i_playbacks ].SamplePlaybackWeighted( context, instance, output, m_applyMotion );
	}
	else if ( poseWeight < 0.99f )
	{
		// Blend animations and cached pose
		instance[ i_playbacks ].SamplePlaybackWeighted( context, instance, output, m_applyMotion );

		CAllocatedBehaviorGraphOutput& pose = instance[ i_pose ];
		SBehaviorGraphOutput* cachedPose = pose.GetPose();
		output.SetInterpolateWithoutME( output, *cachedPose, poseWeight );
	}
	else
	{
		// Only cached pose
		CAllocatedBehaviorGraphOutput& pose = instance[ i_pose ];
		SBehaviorGraphOutput* cachedPose = pose.GetPose();

		output = *cachedPose;
	}
}

void CBehaviorGraphRecentlyUsedAnimsStateNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	// Create and cache pose
	const SBehaviorUsedAnimations& recentlyUsedAnims = instance.GetAnimatedComponent()->GetRecentlyUsedAnims();
	instance[ i_playbacks ].SetupPlayback( recentlyUsedAnims );
	// if only normal animations are used, we don't need to use cached pose
	instance[ i_poseWeight ] = m_poseBlendOutTime > 0.0f && recentlyUsedAnims.IsUsingOnlyNormalAnims() ? 0.0f : 1.0f;

	// Create and cache pose
	CreateAndCachePose( instance );
}

void CBehaviorGraphRecentlyUsedAnimsStateNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	// Destroy pose
	DestroyPose( instance );

	// Create and cache pose
	instance[ i_playbacks ].ClearAnims();

	TBaseClass::OnDeactivated( instance );
}

void CBehaviorGraphRecentlyUsedAnimsStateNode::CreateAndCachePose( CBehaviorGraphInstance& instance ) const
{
	CAllocatedBehaviorGraphOutput& pose = instance[ i_pose ];
	pose.CreateAndCache( instance );
}

void CBehaviorGraphRecentlyUsedAnimsStateNode::DestroyPose( CBehaviorGraphInstance& instance ) const
{
	CAllocatedBehaviorGraphOutput& pose = instance[ i_pose ];
	pose.Free( instance );
}

void CBehaviorGraphRecentlyUsedAnimsStateNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	instance[ i_playbacks ].GetSyncInfo( info );
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
