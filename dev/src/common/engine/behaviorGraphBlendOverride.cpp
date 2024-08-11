/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphBlendNode.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphBlendOverride.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "cacheBehaviorGraphOutput.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "animSyncInfo.h"
#include "behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphBlendOverrideNode );

const Float CBehaviorGraphBlendOverrideNode::ACTIVATION_THRESHOLD = 0.001f;

CBehaviorGraphBlendOverrideNode::CBehaviorGraphBlendOverrideNode()
	: m_synchronize( true ) 
	, m_syncMethod( NULL )
	, m_synchronizeInputFromParent( false ) 
	, m_synchronizeOverrideFromParent( false ) 
	, m_syncMethodFromParent( NULL )
	, m_alwaysActiveOverrideInput( false )
	, m_lodAtOrAboveLevel( BL_NoLod )
	, m_getDeltaMotionFromOverride( false )
{
}

void CBehaviorGraphBlendOverrideNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_controlValue;
	compiler << i_prevControlValue;
}

void CBehaviorGraphBlendOverrideNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_controlValue ] = 0.f;
	instance[ i_prevControlValue ] = 0.f;
}

void CBehaviorGraphBlendOverrideNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_controlValue );
	INST_PROP( i_prevControlValue );
}

TDynArray<SBehaviorGraphBoneInfo>* CBehaviorGraphBlendOverrideNode::GetBonesProperty()
{
	return &m_bones;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphBlendOverrideNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Base ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Override ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
}

Bool CBehaviorGraphBlendOverrideNode::WorkWithLod( EBehaviorLod lod ) const
{
	return lod <= m_lodAtOrAboveLevel;
}

#endif

void CBehaviorGraphBlendOverrideNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheBlock( TXT("Base") );
	m_cachedOverrideInputNode = CacheBlock( TXT("Override") );
	m_cachedControlVariableNode = CacheValueBlock( TXT("Weight") );
}

void CBehaviorGraphBlendOverrideNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	// update variable 
	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Update( context, instance, timeDelta );
	}

	BEH_NODE_UPDATE( BlendOverride );

	// copy variable value (so it's constant across update and sample)
	UpdateControlValue( instance );

	// process activations
	ProcessActivations( instance );

	// synchronize children playback
	Synchronize( instance, timeDelta );

	// update appropriate child nodes
	if ( instance[ i_controlValue ] > ACTIVATION_THRESHOLD || m_alwaysActiveOverrideInput )
	{
		if ( m_cachedInputNode ) 
		{
			ASSERT( m_cachedInputNode->IsActive( instance ) );

			m_cachedInputNode->Update( context, instance, timeDelta );
		}

		if ( m_cachedOverrideInputNode )
		{
			ASSERT( m_cachedOverrideInputNode->IsActive( instance ) );

			m_cachedOverrideInputNode->Update( context, instance, timeDelta );
		}
	}
	else
	{
		if ( m_cachedInputNode ) 
		{
			ASSERT( m_cachedInputNode->IsActive( instance ) );
			ASSERT( !m_cachedOverrideInputNode || ( m_cachedOverrideInputNode && !m_cachedOverrideInputNode->IsActive( instance ) ) );

			m_cachedInputNode->Update( context, instance, timeDelta );
		}
	}
}

void CBehaviorGraphBlendOverrideNode::UpdateControlValue( CBehaviorGraphInstance& instance ) const
{
	instance[ i_prevControlValue ]	= instance[ i_controlValue ];
	instance[ i_controlValue ]		= m_cachedControlVariableNode ? Clamp( m_cachedControlVariableNode->GetValue( instance ), 0.0f, 1.0f ) : 0.0f;
}

void CBehaviorGraphBlendOverrideNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	CHECK_SAMPLE_ID;

	BEH_NODE_SAMPLE( BlendOverride );

	ANIM_NODE_PRE_SAMPLE

	Float controlValue = instance[ i_controlValue ];

	if ( controlValue > ACTIVATION_THRESHOLD || m_alwaysActiveOverrideInput )
	{
		if ( m_cachedInputNode ) 
		{
			m_cachedInputNode->Sample( context, instance, output );
		}

		// Get pose
		CCacheBehaviorGraphOutput cachePose( context );
		SBehaviorGraphOutput* poseOverride = cachePose.GetPose();

		if ( m_cachedOverrideInputNode )
		{
			ASSERT( m_cachedOverrideInputNode->IsActive( instance ) );

			m_cachedOverrideInputNode->Sample( context, instance, *poseOverride );
		}

		Int32 outputBoneNum = output.m_numBones;
		Int32 poseOverrideBoneNum = poseOverride->m_numBones;

#ifdef DISABLE_SAMPLING_AT_LOD3
		if ( context.GetLodLevel() <= BL_Lod2 )
#endif
		{
			// Override pose
			for (Uint32 i=0; i<m_bones.Size(); i++)
			{
				Int32 outputIndex = m_bones[i].m_num;

				if ( outputIndex < outputBoneNum && outputIndex < poseOverrideBoneNum )
				{
#ifdef USE_HAVOK_ANIMATION
					hkReal weight = m_bones[i].m_weight * controlValue;
#else
					Float weight = m_bones[i].m_weight * controlValue;
#endif
					if ( weight > 0.f )
					{
#ifdef USE_HAVOK_ANIMATION
						output.m_outputPose[outputIndex].setInterpolate4( output.m_outputPose[outputIndex], poseOverride->m_outputPose[outputIndex], weight );
#else
						output.m_outputPose[outputIndex].Lerp( output.m_outputPose[outputIndex], poseOverride->m_outputPose[outputIndex], weight );
#endif
					}
				}	
			}
		}

		// Copy float track
		for (Uint32 i=0; i<output.m_numFloatTracks && i<poseOverride->m_numFloatTracks; i++ )
		{
			output.m_floatTracks[i] = ::Max<Float>(output.m_floatTracks[i], poseOverride->m_floatTracks[i]);
		}

		// Copy delta motion if requested
		if ( m_getDeltaMotionFromOverride )
		{
			output.m_deltaReferenceFrameLocal.Slerp( output.m_deltaReferenceFrameLocal, poseOverride->m_deltaReferenceFrameLocal, controlValue );
		}

		// Copy custom track
		COMPILE_ASSERT( SBehaviorGraphOutput::NUM_CUSTOM_FLOAT_TRACKS == 5 );
		output.m_customFloatTracks[0] = ::Max<Float>( output.m_customFloatTracks[0], poseOverride->m_customFloatTracks[0] );
		output.m_customFloatTracks[1] = ::Max<Float>( output.m_customFloatTracks[1], poseOverride->m_customFloatTracks[1] );
		output.m_customFloatTracks[2] = ::Max<Float>( output.m_customFloatTracks[2], poseOverride->m_customFloatTracks[2] );
		output.m_customFloatTracks[3] = ::Max<Float>( output.m_customFloatTracks[3], poseOverride->m_customFloatTracks[3] );
		output.m_customFloatTracks[4] = ::Max<Float>( output.m_customFloatTracks[4], poseOverride->m_customFloatTracks[4] );

		// Merge events
		output.MergeEventsAndUsedAnimsAsOverlays( *poseOverride, controlValue );
	}
	else if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->Sample( context, instance, output );
	}

	ANIM_NODE_POST_SAMPLE
}

void CBehaviorGraphBlendOverrideNode::Synchronize( CBehaviorGraphInstance& instance, Float timeDelta ) const
{	
	// base animation is played with normal speed
	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->GetSyncData( instance ).Reset();
	}

	// added input is synchronized to match base animation, 
	// no matter for alpha
	if ( instance[ i_controlValue ] > ACTIVATION_THRESHOLD )
	{
		if ( m_synchronize && m_syncMethod )
		{
			const Float alphaZero = 0.0f;
			m_syncMethod->Synchronize( instance, m_cachedInputNode, m_cachedOverrideInputNode, alphaZero, timeDelta );	
		}
	}
}

void CBehaviorGraphBlendOverrideNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{	
	if ( m_synchronizeInputFromParent && m_cachedInputNode ) 
	{
		m_cachedInputNode->GetSyncInfo( instance, info );
	}
	else if ( m_synchronizeOverrideFromParent && m_cachedOverrideInputNode ) 
	{
		m_cachedOverrideInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphBlendOverrideNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_synchronizeInputFromParent && m_cachedInputNode )
	{		
		if ( m_syncMethodFromParent )
		{
			m_syncMethodFromParent->SynchronizeTo( instance, m_cachedInputNode, info );
		}
		else
		{
			m_cachedInputNode->SynchronizeTo( instance, info );
		}
	}
	if ( m_synchronizeOverrideFromParent && m_cachedOverrideInputNode )
	{		
		if ( m_syncMethodFromParent )
		{
			m_syncMethodFromParent->SynchronizeTo( instance, m_cachedOverrideInputNode, info );
		}
		else
		{
			m_cachedOverrideInputNode->SynchronizeTo( instance, info );
		}
	}
}

void CBehaviorGraphBlendOverrideNode::ProcessActivations( CBehaviorGraphInstance& instance ) const
{
	Bool isPrevActive = instance[ i_prevControlValue ] > ACTIVATION_THRESHOLD ? true : false;
	Bool isCurrActive = instance[ i_controlValue ] > ACTIVATION_THRESHOLD ? true : false;

	if ( m_cachedOverrideInputNode )
	{
		if ( !isPrevActive && isCurrActive )
		{	
			m_cachedOverrideInputNode->Activate( instance );

			if ( m_cachedInputNode && m_synchronize && m_syncMethod )
			{
				CSyncInfo syncInfo;
				m_cachedInputNode->GetSyncInfo( instance, syncInfo );
				m_syncMethod->SynchronizeTo( instance, m_cachedOverrideInputNode, syncInfo );
			}
		}

		if ( ( isPrevActive && !isCurrActive ) && !m_alwaysActiveOverrideInput )
		{	
			m_cachedOverrideInputNode->Deactivate( instance );
		}
	}
}

Bool CBehaviorGraphBlendOverrideNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool retVal = false;

	if ( m_cachedInputNode && m_cachedInputNode->ProcessEvent( instance, event ) ) 
	{
		retVal = true;
	}
	
	// Pass to override input
	if ( m_cachedOverrideInputNode && m_cachedOverrideInputNode->ProcessEvent( instance, event ) )
	{
		retVal = true;
	}

	return retVal;
}

void CBehaviorGraphBlendOverrideNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Activate( instance );
	}

	UpdateControlValue( instance );

	// activate inputs
	if ( instance[ i_controlValue ] > ACTIVATION_THRESHOLD || m_alwaysActiveOverrideInput )
	{
		if ( m_cachedInputNode ) 
		{
			m_cachedInputNode->Activate( instance );
		}

		if ( m_cachedOverrideInputNode )
		{
			m_cachedOverrideInputNode->Activate( instance );
		}
	}
	else
	{
		if ( m_cachedInputNode ) 
		{
			m_cachedInputNode->Activate( instance );
		}
	}
}

void CBehaviorGraphBlendOverrideNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	instance[ i_controlValue ] = 0.f;

	// we can safely deactivate both inputs
	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->Deactivate( instance );
	}

	if ( m_cachedOverrideInputNode ) 
	{
		m_cachedOverrideInputNode->Deactivate( instance );
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Deactivate( instance );
	}
}

void CBehaviorGraphBlendOverrideNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	const Float controlValue = instance[ i_controlValue ];

	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( controlValue > ACTIVATION_THRESHOLD || m_alwaysActiveOverrideInput )
	{
		if ( m_cachedOverrideInputNode )
		{
			m_cachedOverrideInputNode->ProcessActivationAlpha( instance, m_alwaysActiveOverrideInput? alpha : alpha * controlValue );
		}
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->ProcessActivationAlpha( instance, alpha );
	}
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
