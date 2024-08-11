/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphFrozenStateNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "allocatedBehaviorGraphOutput.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "behaviorGraphContext.h"
#include "graphConnectionRebuilder.h"
#include "behaviorGraphSocket.h"
#include "cacheBehaviorGraphOutput.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphFrozenStateNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphFrozenStateNode::GetCaption() const
{
	return !m_name.Empty() ? String::Printf( TXT("Frozen state - %s"), m_name.AsChar() ) : TXT("Frozen state");
}

Color CBehaviorGraphFrozenStateNode::GetTitleColor() const
{
	return Color( 143, 170, 190 );
}

Bool CBehaviorGraphFrozenStateNode::CanBeExpanded() const
{
	return false;
}

#endif

void CBehaviorGraphFrozenStateNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_pose;
}

void CBehaviorGraphFrozenStateNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );
}

void CBehaviorGraphFrozenStateNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReleaseInstance( instance );

	DestroyPose( instance );
}

void CBehaviorGraphFrozenStateNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	// Write cached pose to output
	CAllocatedBehaviorGraphOutput& pose = instance[ i_pose ];

	SBehaviorGraphOutput* cachedPose = pose.GetPose();
	if ( cachedPose )
	{
		output = *cachedPose;
	}
}

void CBehaviorGraphFrozenStateNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	// Create and cache pose
	CreateAndCachePose( instance );
}

void CBehaviorGraphFrozenStateNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	// Destroy pose
	DestroyPose( instance );

	TBaseClass::OnDeactivated( instance );
}

void CBehaviorGraphFrozenStateNode::CreateAndCachePose( CBehaviorGraphInstance& instance ) const
{
	CAllocatedBehaviorGraphOutput& pose = instance[ i_pose ];
	pose.CreateAndCache( instance );
}

void CBehaviorGraphFrozenStateNode::DestroyPose( CBehaviorGraphInstance& instance ) const
{
	CAllocatedBehaviorGraphOutput& pose = instance[ i_pose ];
	pose.Free( instance );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphPoseMemoryNode );

CBehaviorGraphPoseMemoryNode::CBehaviorGraphPoseMemoryNode()
	: m_blendOutDuration( 0.2f )
{

}

void CBehaviorGraphPoseMemoryNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_pose;
	compiler << i_blendOutEventId;
	compiler << i_blendOutTimer;
	//compiler << i_blendOutEventOccured;
}

void CBehaviorGraphPoseMemoryNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );

	instance[ i_blendOutEventId ] = instance.GetEventId( m_blendOutEvent );
	
	InternalReset( instance );
}

void CBehaviorGraphPoseMemoryNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReleaseInstance( instance );

	DestroyPose( instance );
}

void CBehaviorGraphPoseMemoryNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	Float& blendOutTimer = instance[ i_blendOutTimer ];
	if ( blendOutTimer > 0.f )
	{
		blendOutTimer = Max( 0.f, blendOutTimer - timeDelta );
	}
}

void CBehaviorGraphPoseMemoryNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	// Write cached pose to output
	CAllocatedBehaviorGraphOutput& pose = instance[ i_pose ];
	SBehaviorGraphOutput* cachedPose = pose.GetPose();
	if ( cachedPose )
	{
		const Float blendOutTimer = instance[ i_blendOutTimer ];
		if ( blendOutTimer > 0.f )
		{
			const Float w = blendOutTimer / m_blendOutDuration;
			RED_FATAL_ASSERT( w >= 0.f && w <= 1.f, "ANIM FATAL ERROR" );

			//CCacheBehaviorGraphOutput tempCachedPose( context, IsMimic() );
			//SBehaviorGraphOutput* tempPose = tempCachedPose.GetPose();
			//if ( tempPose )
			{
				if ( IsMimic() )
				{
					for( Uint32 i=0; i<output.m_numFloatTracks; ++i )
					{
						output.m_floatTracks[i] += w * cachedPose->m_floatTracks[i];
					}
				}
				//else
				//{
				//	tempPose->SetIdentity();
				//	output.SetInterpolate( *tempPose, *cachedPose, w );
				//}
			}
		}
		else
		{
			*cachedPose = output;
		}
	}
}

void CBehaviorGraphPoseMemoryNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	InternalReset( instance );

	// Create and cache pose
	CreateAndCachePose( instance );
}

void CBehaviorGraphPoseMemoryNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	// Destroy pose
	DestroyPose( instance );

	TBaseClass::OnDeactivated( instance );
}

void CBehaviorGraphPoseMemoryNode::CreateAndCachePose( CBehaviorGraphInstance& instance ) const
{
	CAllocatedBehaviorGraphOutput& pose = instance[ i_pose ];
	pose.CreateAndCache( instance, IsMimic() );
}

void CBehaviorGraphPoseMemoryNode::DestroyPose( CBehaviorGraphInstance& instance ) const
{
	CAllocatedBehaviorGraphOutput& pose = instance[ i_pose ];
	pose.Free( instance );
}

CBehaviorGraph* CBehaviorGraphPoseMemoryNode::GetParentGraph()
{
	return GetGraph();
}

Bool CBehaviorGraphPoseMemoryNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool ret = TBaseClass::ProcessEvent( instance, event );

	if ( event.GetEventID() == instance[ i_blendOutEventId ] )
	{
		StartBlendOutTimer( instance );
		ret = true;
	}

	return ret;
}

void CBehaviorGraphPoseMemoryNode::StartBlendOutTimer( CBehaviorGraphInstance& instance ) const
{
	instance[ i_blendOutTimer ] = m_blendOutDuration;
}

void CBehaviorGraphPoseMemoryNode::InternalReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_blendOutTimer ] = 0.f;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphPoseMemoryNode_Mimic );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphPoseMemoryNode_Mimic::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );
	CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
}

#endif

void CBehaviorGraphPoseMemoryNode_Mimic::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheMimicBlock( TXT("Input") );
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
