/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphMimicManualSlot.h"
#include "cacheBehaviorGraphOutput.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphSocket.h"
#include "behaviorGraphContext.h"
#include "../engine/graphConnectionRebuilder.h"
#include "behaviorGraphOutput.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicManualSlotNode );

void CBehaviorGraphMimicManualSlotNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	if ( context.HasMimic() == false )
	{
		return;
	}

	const Float inputWeight = ( 1.f - instance[ i_nodeWeight ] );
	ASSERT( inputWeight >= 0.f && inputWeight <= 1.f );

	const Bool blendWithInput = inputWeight > 0.f;
	if ( blendWithInput )
	{
		SampleWhenNoAnimation( instance, context, output );

		// only if input is not at 100%
		if ( inputWeight < 1.0f )
		{
			CCacheBehaviorGraphOutput cachePoseForSlot( context, true );
			SBehaviorGraphOutput* poseForSlot = cachePoseForSlot.GetPose();

			if ( poseForSlot && SamplePoseFromSlot( context, instance, *poseForSlot ) )
			{
				// merge/interpolate only if slot was actually used
#ifdef DISABLE_SAMPLING_AT_LOD3
				if ( context.GetLodLevel() <= BL_Lod2 )
				{
					output.SetInterpolate( *poseForSlot, output, inputWeight );
				}
				else
				{
					output.SetInterpolateME( *poseForSlot, output, inputWeight );
				}
#else
				output.SetInterpolate( *poseForSlot, output, inputWeight );
#endif

				output.MergeEventsAndUsedAnims( *poseForSlot, 1.f - inputWeight );
			}
		}
	}
	else
	{
		if ( ! SamplePoseFromSlot( context, instance, output ) )
		{
			// well, there was no animation to be played?
			SampleWhenNoAnimation( instance, context, output );
		}
	}
}

void CBehaviorGraphMimicManualSlotNode::PerformPoseCorrection( SBehaviorSampleContext& context, SBehaviorGraphOutput& output ) const
{
//	TBaseClass::PerformPoseCorrection( context, output );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphMimicManualSlotNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Animation ) ) );
}

String CBehaviorGraphMimicManualSlotNode::GetCaption() const
{
	return String::Printf( TXT("Mimic manual slot [ %s ]"), m_name.AsChar() );
}

Color CBehaviorGraphMimicManualSlotNode::GetTitleColor() const
{
	return Color( 128, 0, 128 );
}
#endif