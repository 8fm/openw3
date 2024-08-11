/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphFlowNode.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphSocket.h"
#include "behaviorGraphContext.h"
#include "../engine/graphConnectionRebuilder.h"
#include "skeleton.h"
#include "animatedComponent.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphInputNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphInputNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );
}

String CBehaviorGraphInputNode::GetCaption() const
{
	return TXT("Input");
}

Color CBehaviorGraphInputNode::GetTitleColor() const
{
	return Color( 64, 255, 64 );
}

#endif

void CBehaviorGraphInputNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	output = context.GetPoseFromPrevSampling();

	if ( context.ShouldCorrectPose() )
	{
		context.SetPoseCorrection( output );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphTPoseNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphTPoseNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );
}

String CBehaviorGraphTPoseNode::GetCaption() const
{
	return TXT("TPose");
}

Color CBehaviorGraphTPoseNode::GetTitleColor() const
{
	return Color( 64, 255, 64 );
}

#endif

void CBehaviorGraphTPoseNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	if ( const CSkeleton* s = instance.GetAnimatedComponent()->GetSkeleton() )
	{
		s->CopyReferencePoseLSTo( output.m_numBones, output.m_outputPose );
	}

	if ( context.ShouldCorrectPose() )
	{
		context.SetPoseCorrection( output );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphIdentityPoseNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphIdentityPoseNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );
}

String CBehaviorGraphIdentityPoseNode::GetCaption() const
{
	return TXT("Identity Pose");
}

Color CBehaviorGraphIdentityPoseNode::GetTitleColor() const
{
	return Color( 64, 255, 64 );
}

#endif

void CBehaviorGraphIdentityPoseNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	output.SetIdentity();
}
