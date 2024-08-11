
#include "build.h"
#include "behaviorGraphMimicLipsync.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "cacheBehaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"

#include "../core/instanceDataLayoutCompiler.h"
#include "animatedComponent.h"
#include "skeleton.h"
#include "behaviorProfiler.h"


IMPLEMENT_ENGINE_CLASS( CBehaviorGraphMimicLipsyncFilterNode );

CBehaviorGraphMimicLipsyncFilterNode::CBehaviorGraphMimicLipsyncFilterNode()
	: m_lipsyncControlTrack( -1 )
	, m_lipsyncTrackBeginA( 1 )
	, m_lipsyncTrackEndA( 33 )
	, m_lipsyncTrackBeginB( 58 )
	, m_lipsyncTrackEndB( 66 )
	, m_lipsyncTrackBeginC( -1 )
	, m_lipsyncTrackEndC( -1 )
{
	
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphMimicLipsyncFilterNode::OnRebuildSockets()
{
	TBaseClass::OnRebuildSockets();

	CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( CNAME( Filter ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
}

#endif

void CBehaviorGraphMimicLipsyncFilterNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_valid;
}

void CBehaviorGraphMimicLipsyncFilterNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	Bool& valid = instance[ i_valid ];
	valid = false;

	const CSkeleton* skeleton = instance.GetAnimatedComponent()->GetMimicSkeleton();
	if ( skeleton )
	{
		const Int32 numTracks = skeleton->GetTracksNum();

		const Bool controlValid = m_lipsyncControlTrack != -1 && m_lipsyncControlTrack < numTracks;
		const Bool trackValidA = m_lipsyncTrackBeginA != -1 && m_lipsyncTrackBeginA < numTracks && m_lipsyncTrackEndA != -1 && m_lipsyncTrackEndA < numTracks;
		const Bool trackValidB = m_lipsyncTrackBeginB != -1 && m_lipsyncTrackBeginB < numTracks && m_lipsyncTrackEndB != -1 && m_lipsyncTrackEndB < numTracks;
		const Bool trackValidC = m_lipsyncTrackBeginC != -1 && m_lipsyncTrackBeginC < numTracks && m_lipsyncTrackEndC != -1 && m_lipsyncTrackEndC < numTracks;

		valid = controlValid && trackValidA && trackValidB && trackValidC;
	}
}

void CBehaviorGraphMimicLipsyncFilterNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( MimicLipsyncFilter );
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedFilterInputNode )
	{
		m_cachedFilterInputNode->Update( context, instance, timeDelta );
	}

	if ( m_cachedWeightValueNode )
	{
		m_cachedWeightValueNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphMimicLipsyncFilterNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	const Bool valid = instance[ i_valid ];

	if ( m_cachedFilterInputNode )
	{
		CCacheBehaviorGraphOutput cacheFilterPose( context, true );

		SBehaviorGraphOutput* filterPose = cacheFilterPose.GetPose();
		if ( filterPose && (Int32)filterPose->m_numFloatTracks > m_lipsyncControlTrack && (Int32)output.m_numFloatTracks > m_lipsyncControlTrack )
		{
			const Int32 size = (Int32)output.m_numFloatTracks;

			m_cachedFilterInputNode->Sample( context, instance, *filterPose );

			Float controlTrackValue = output.m_floatTracks[ m_lipsyncControlTrack ];

			if ( m_cachedWeightValueNode )
			{
				controlTrackValue = Clamp ( controlTrackValue * m_cachedWeightValueNode->GetValue( instance ), 0.f, 1.f );
			}

			if ( valid )
			{
				const Float lipsyncControlWeight = 1.f - controlTrackValue;

				for ( Int32 i=m_lipsyncTrackBeginA; i<=m_lipsyncTrackEndA; ++i )
				{
					filterPose->m_floatTracks[ i ] *= lipsyncControlWeight;
				}

				for ( Int32 i=m_lipsyncTrackBeginB; i<=m_lipsyncTrackEndB; ++i )
				{
					filterPose->m_floatTracks[ i ] *= lipsyncControlWeight;
				}

				for ( Int32 i=m_lipsyncTrackBeginC; i<=m_lipsyncTrackEndC; ++i )
				{
					filterPose->m_floatTracks[ i ] *= lipsyncControlWeight;
				}
			}

			for ( Int32 i=0; i<size; i++ )
			{
				output.m_floatTracks[ i ] += filterPose->m_floatTracks[ i ];
			}

			for ( Uint32 i=0; i<output.m_numBones; ++i )
			{
#ifdef USE_HAVOK_ANIMATION
				output.m_outputPose[ i ].setMul( output.m_outputPose[ i ], filterPose->m_outputPose[ i ] );
#else
				output.m_outputPose[ i ].SetMul( output.m_outputPose[ i ], filterPose->m_outputPose[ i ] );
#endif
			}

			output.MergeEvents( *filterPose );
		}
	}	
}

void CBehaviorGraphMimicLipsyncFilterNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedFilterInputNode )
	{
		m_cachedFilterInputNode->Activate( instance );
	}

	if ( m_cachedWeightValueNode )
	{
		m_cachedWeightValueNode->Activate( instance );
	}
}

void CBehaviorGraphMimicLipsyncFilterNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedFilterInputNode )
	{
		m_cachedFilterInputNode->Deactivate( instance );
	}

	if ( m_cachedWeightValueNode )
	{
		m_cachedWeightValueNode->Deactivate( instance );
	}
}

void CBehaviorGraphMimicLipsyncFilterNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedWeightValueNode = CacheValueBlock( TXT("Weight") );
	m_cachedFilterInputNode = CacheMimicBlock( TXT("Filter") );
}

Bool CBehaviorGraphMimicLipsyncFilterNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool ret = TBaseClass::ProcessEvent( instance, event );

	if ( m_cachedFilterInputNode )
	{
		ret |= m_cachedFilterInputNode->ProcessEvent( instance, event );
	}

	return ret;
}

void CBehaviorGraphMimicLipsyncFilterNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedFilterInputNode )
	{
		m_cachedFilterInputNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedWeightValueNode )
	{
		m_cachedWeightValueNode->ProcessActivationAlpha( instance, alpha );
	}
}
