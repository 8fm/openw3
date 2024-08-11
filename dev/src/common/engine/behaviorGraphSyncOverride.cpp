
#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphSyncOverride.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "cacheBehaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "behaviorGraphContext.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/animatedIterators.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphSyncOverrideNode );

CBehaviorGraphSyncOverrideNode::CBehaviorGraphSyncOverrideNode()
	: m_defaultWeight( 0.5f )
	, m_blendRootParent( false )
{
}

void CBehaviorGraphSyncOverrideNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_bones;
	compiler << i_boneRoot;
	compiler << i_controlValue;
	compiler << i_prevControlValue;
}

void CBehaviorGraphSyncOverrideNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	Int32 rootBone = FindBoneIndex( m_rootBoneName, instance );
	if ( rootBone != -1 )
	{
		FillBones( rootBone, instance[ i_bones ], instance );
	}

	instance[ i_boneRoot ] = rootBone;
	instance[ i_controlValue ] = 0.f;
	instance[ i_prevControlValue ] = 0.f;
}

void CBehaviorGraphSyncOverrideNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_boneRoot );
	INST_PROP( i_controlValue );
	INST_PROP( i_prevControlValue );
	INST_PROP( i_bones );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphSyncOverrideNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );	
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Base ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Override ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
}

#endif

void CBehaviorGraphSyncOverrideNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheBlock( TXT("Base") );
	m_cachedOverrideInputNode = CacheBlock( TXT("Override") );
	m_cachedControlVariableNode = CacheValueBlock( TXT("Weight") );
}

void CBehaviorGraphSyncOverrideNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( BlendSyncOverride );

	// Update variable 
	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Update( context, instance, timeDelta );
	}

	// Copy variable value (so it's constant across update and sample)
	UpdateControlValue( instance );

	// Process activations
	ProcessActivations( instance );

	// update appropriate child nodes
	if ( IsOverrideActive( instance ) )
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

void CBehaviorGraphSyncOverrideNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	CHECK_SAMPLE_ID;

	BEH_NODE_SAMPLE( BlendSyncOverride );

	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->Sample( context, instance, output );
	}

	if ( m_cachedOverrideInputNode && IsOverrideActive( instance ) )
	{
		// Get pose
		CCacheBehaviorGraphOutput cachePose( context );
		SBehaviorGraphOutput* poseOverride = cachePose.GetPose();
		if ( poseOverride )
		{
			const Float controlValue = instance[ i_controlValue ];

			ASSERT( m_cachedOverrideInputNode->IsActive( instance ) );
			m_cachedOverrideInputNode->Sample( context, instance, *poseOverride );

			// Pose
#ifdef DISABLE_SAMPLING_AT_LOD3
			if ( context.GetLodLevel() <= BL_Lod2 )
#endif
			{
				const Int32 outputBoneNum = (Int32)output.m_numBones;
				ASSERT( outputBoneNum == (Int32)poseOverride->m_numBones );

				// First part
				// Do nothing, pose is already in output

				// Second part
				TDynArray< Int32 >& bones = instance[ i_bones ];
				
				const Uint32 boneNum = bones.Size();
				for ( Uint32 i=0; i<boneNum; ++i )
				{
					const Int32 bone = bones[ i ];
					ASSERT( bone < outputBoneNum );
					output.m_outputPose[ bone ] = poseOverride->m_outputPose[ bone ];
				}

				const Int32 rootBone = instance[ i_boneRoot ];
				ASSERT( rootBone != -1 && rootBone < outputBoneNum );

				// Connect two poses into one part
				ConnectTwoPoses( output, *poseOverride, rootBone, instance );
			}
			
			// We don't interpolate float and custom tracks

			// Merge events
			output.MergeEventsAndUsedAnimsAsOverlays( *poseOverride, controlValue );
		}
	}
}

void CBehaviorGraphSyncOverrideNode::ConnectTwoPoses( SBehaviorGraphOutput &a, SBehaviorGraphOutput &b, Uint32 bone, CBehaviorGraphInstance& instance ) const
{
	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	const CSkeleton* skeleton = ac ? ac->GetSkeleton() : NULL;
	if ( skeleton )
	{
		Int32 parent = skeleton->GetParentIndices()[ bone ];
		if ( parent == -1 )
		{
			return;
		}

		if ( m_blendRootParent )
		{
#ifdef USE_HAVOK_ANIMATION
			a.m_outputPose[ parent ].setInterpolate4( a.m_outputPose[ parent ], b.m_outputPose[ parent ], m_defaultWeight );
#else
			a.m_outputPose[ parent ].Lerp( a.m_outputPose[ parent ], b.m_outputPose[ parent ], m_defaultWeight );
#endif
		}
		AnimQsTransform boneParentAMS = a.GetBoneModelTransform( parent, skeleton->GetParentIndices() );
		AnimQsTransform boneBMS = b.GetBoneModelTransform( bone, skeleton->GetParentIndices() );

		AnimQsTransform final;
#ifdef USE_HAVOK_ANIMATION
		final.setMulInverseMul( boneParentAMS, boneBMS );
#else
		final.SetMulInverseMul( boneParentAMS, boneBMS );
#endif
		// Write final bone rotation to pose 'a'
#ifdef USE_HAVOK_ANIMATION
		a.m_outputPose[ bone ].m_rotation = final.m_rotation;
#else
		a.m_outputPose[ bone ].Rotation = final.Rotation;
#endif
	}
}

void CBehaviorGraphSyncOverrideNode::FillBones( Int32 rootBone, TDynArray< Int32 >& bones, CBehaviorGraphInstance& instance ) const
{
	for ( BoneChildrenIterator it( instance.GetAnimatedComponent(), rootBone ); it; ++it )
	{
		bones.PushBack( *it );
	}
}

Bool CBehaviorGraphSyncOverrideNode::IsOverrideActive( CBehaviorGraphInstance& instance ) const
{
	return IsActive( instance[ i_controlValue ] );
}

void CBehaviorGraphSyncOverrideNode::UpdateControlValue( CBehaviorGraphInstance& instance ) const
{
	instance[ i_prevControlValue ]	= instance[ i_controlValue ];
	instance[ i_controlValue ]		= m_cachedControlVariableNode ? Clamp( m_cachedControlVariableNode->GetValue( instance ), 0.0f, 1.0f ) : m_defaultWeight;
}

void CBehaviorGraphSyncOverrideNode::ProcessActivations( CBehaviorGraphInstance& instance ) const
{
	Bool isPrevActive = IsActive( instance[ i_prevControlValue ] );
	Bool isCurrActive = IsActive( instance[ i_controlValue ] );

	if ( m_cachedOverrideInputNode )
	{
		if ( !isPrevActive && isCurrActive )
		{	
			m_cachedOverrideInputNode->Activate( instance );
		}
		else if ( isPrevActive && !isCurrActive )
		{	
			m_cachedOverrideInputNode->Deactivate( instance );
		}
	}
}

void CBehaviorGraphSyncOverrideNode::Synchronize( CBehaviorGraphInstance& instance, Float timeDelta ) const
{	
	
}

void CBehaviorGraphSyncOverrideNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{	
	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphSyncOverrideNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphSyncOverrideNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool ret = false;

	if ( m_cachedInputNode ) 
	{
		ret |= m_cachedInputNode->ProcessEvent( instance, event );
	}

	if ( m_cachedOverrideInputNode )
	{
		ret |= m_cachedOverrideInputNode->ProcessEvent( instance, event );
	}

	return ret;
}

void CBehaviorGraphSyncOverrideNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	UpdateControlValue( instance );

	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->Activate( instance );
	}

	if ( m_cachedOverrideInputNode && IsOverrideActive( instance ) )
	{
		m_cachedOverrideInputNode->Activate( instance );
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->Activate( instance );
	}
}

void CBehaviorGraphSyncOverrideNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
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

void CBehaviorGraphSyncOverrideNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	const Float controlValue = instance[ i_controlValue ];

	if ( m_cachedInputNode ) 
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedOverrideInputNode && IsOverrideActive( instance ) )
	{
		m_cachedOverrideInputNode->ProcessActivationAlpha( instance, alpha * controlValue );
	}

	if ( m_cachedControlVariableNode )
	{
		m_cachedControlVariableNode->ProcessActivationAlpha( instance, alpha );
	}
}
