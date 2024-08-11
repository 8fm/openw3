/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphContainerNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphSelfActStateMachine.h"
#include "cacheBehaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/animatedIterators.h"
#include "../engine/graphConnectionRebuilder.h"
#include "behaviorProfiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphDefaultSelfActStateNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool CBehaviorGraphDefaultSelfActStateNode::CanBeExpanded() const
{
	return false;
}

#endif

void CBehaviorGraphDefaultSelfActStateNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	CBehaviorGraphSelfActivatingStateMachineNode* machine = SafeCast< CBehaviorGraphSelfActivatingStateMachineNode >( GetParentNode() );
	m_rootNode = machine->GetInputNode();
}

void CBehaviorGraphDefaultSelfActStateNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	// no sync info
}
void CBehaviorGraphDefaultSelfActStateNode:: SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	// nothing to synchronize to
}

Bool CBehaviorGraphDefaultSelfActStateNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	return false;
}

Bool CBehaviorGraphDefaultSelfActStateNode::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	return false;
}

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphSelfActivatingStateMachineNode );

CBehaviorGraphSelfActivatingStateMachineNode::CBehaviorGraphSelfActivatingStateMachineNode()
{
	m_applySyncTags = false;
}

void CBehaviorGraphSelfActivatingStateMachineNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_activateMachine;
	compiler << i_running;
}

void CBehaviorGraphSelfActivatingStateMachineNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	InternalReset( instance );
}

void CBehaviorGraphSelfActivatingStateMachineNode::InternalReset( CBehaviorGraphInstance& instance ) const
{
	instance[ i_activateMachine ] = false;
	instance[ i_running ] = false;

	SwitchToState( m_defaultNode, instance );
}

Bool CBehaviorGraphSelfActivatingStateMachineNode::IsRunning( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_running ];
}

void CBehaviorGraphSelfActivatingStateMachineNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	m_cachedInputNode = CacheBlock( TXT("Input") );
}

CBehaviorGraphNode* CBehaviorGraphSelfActivatingStateMachineNode::GetInputNode() const
{
	return m_cachedInputNode;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphSelfActivatingStateMachineNode::OnSpawned( const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );

	CBehaviorGraphStateNode* state = CreateDefaultStateNode();

	SetDefaultState( state );
}

CBehaviorGraphStateNode* CBehaviorGraphSelfActivatingStateMachineNode::CreateDefaultStateNode()
{
	GraphBlockSpawnInfo spawnInfo( CBehaviorGraphDefaultSelfActStateNode::GetStaticClass() );

	CBehaviorGraphDefaultSelfActStateNode* newNode = SafeCast< CBehaviorGraphDefaultSelfActStateNode >( CreateChildNode( spawnInfo.GetClass() ) );

	return newNode;
}

void CBehaviorGraphSelfActivatingStateMachineNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );

	CBehaviorGraphContainerNode::OnRebuildSockets();
}

String CBehaviorGraphSelfActivatingStateMachineNode::GetCaption() const
{
	if ( !m_name.Empty() )
	{
		return String::Printf( TXT("Self Act State machine [ %s ]"), m_name.AsChar() );
	}
	else
	{
		return String( TXT("Self Act State machine") );
	}
}

#endif

void CBehaviorGraphSelfActivatingStateMachineNode::RequestActivateMachine( CBehaviorGraphInstance& instance ) const
{
	instance[ i_activateMachine ] = true;
}

void CBehaviorGraphSelfActivatingStateMachineNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	BEH_NODE_UPDATE( SelfActivatingStateMachine );
	Bool& activateMachine = instance[ i_activateMachine ];
	Bool& running = instance[ i_running ];

	if ( activateMachine )
	{
		UpdateTransitions( context, instance, timeDelta );

		activateMachine = false;

		running = true;
	}
	else if ( running )
	{
		UpdateTransitions( context, instance, timeDelta );
	}

	CBehaviorGraphNode* currentState = GetCurrentState( instance );

	if ( running && ( !currentState || currentState == m_defaultNode ) )
	{
		running = false;
	}

	UpdateCurrentState( context, instance, timeDelta );
}

Bool CBehaviorGraphSelfActivatingStateMachineNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool ret = TBaseClass::ProcessEvent( instance, event );
	if ( ret )
	{
		RequestActivateMachine( instance );
	}
	return ret;
}

Bool CBehaviorGraphSelfActivatingStateMachineNode::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool ret = TBaseClass::ProcessForceEvent( instance, event );
	if ( ret )
	{
		RequestActivateMachine( instance );
	}
	return ret;
}

void CBehaviorGraphSelfActivatingStateMachineNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	CBehaviorGraphNode* currentState = GetCurrentState( instance );
	if ( currentState ) 
	{
		ASSERT( currentState->IsActive( instance ) );
		currentState->Sample( context, instance, output );
	}
}

void CBehaviorGraphSelfActivatingStateMachineNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	CBehaviorGraphNode* currentState = GetCurrentState( instance );
	if ( currentState && currentState != m_defaultNode ) 
	{
		currentState->GetSyncInfo( instance, info );
	}
	else if ( m_cachedInputNode )
	{
		m_cachedInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphSelfActivatingStateMachineNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	CBehaviorGraphNode* currentState = GetCurrentState( instance );
	if ( currentState && currentState != m_defaultNode ) 
	{
		currentState->SynchronizeTo( instance, info );
	}
	else if ( m_cachedInputNode )
	{
		m_cachedInputNode->SynchronizeTo( instance, info );
	}
}

void CBehaviorGraphSelfActivatingStateMachineNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	InternalReset( instance );
}

void CBehaviorGraphSelfActivatingStateMachineNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );
}

void CBehaviorGraphSelfActivatingStateMachineNode::OnActivated( CBehaviorGraphInstance& instance ) const
{	
	TBaseClass::OnActivated( instance );
}

void CBehaviorGraphSelfActivatingStateMachineNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	InternalReset( instance );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphSelfActivatingOverrideStateMachineNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphSelfActivatingOverrideStateMachineNode::GetCaption() const
{
	if ( !m_name.Empty() )
	{
		return String::Printf( TXT("Self Act State machine with override [ %s ]"), m_name.AsChar() );
	}
	else
	{
		return String( TXT("Self Act State machine with override") );
	}
}

#endif

TDynArray<SBehaviorGraphBoneInfo>* CBehaviorGraphSelfActivatingOverrideStateMachineNode::GetBonesProperty()
{
	return &m_bones;
}

void CBehaviorGraphSelfActivatingOverrideStateMachineNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	BEH_NODE_UPDATE( SelfActivatingOverrideStateMachine );
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphSelfActivatingOverrideStateMachineNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	if ( !m_cachedInputNode )
	{
		return;
	}

	CBehaviorGraphNode* currentState = GetCurrentState( instance );
	if ( currentState ) 
	{
		ASSERT( currentState->IsActive( instance ) );

		if ( IsRunning( instance ) )
		{
			ASSERT( m_cachedInputNode->IsActive( instance ) );

			CCacheBehaviorGraphOutput cachePose( context );
			SBehaviorGraphOutput* poseOverride = cachePose.GetPose();

			if ( poseOverride )
			{
				m_cachedInputNode->Sample( context, instance, output );
				currentState->Sample( context, instance, *poseOverride );

				Int32 outputBoneNum = output.m_numBones;
				Int32 poseOverrideBoneNum = poseOverride->m_numBones;

				for (Uint32 i=0; i<m_bones.Size(); i++)
				{
					Int32 outputIndex = m_bones[i].m_num;

					if ( outputIndex < outputBoneNum && outputIndex < poseOverrideBoneNum )
					{
#ifdef USE_HAVOK_ANIMATION
						hkReal weight = m_bones[i].m_weight;
						if ( weight > 0.f )
						{
							output.m_outputPose[ outputIndex ].setInterpolate4( output.m_outputPose[ outputIndex ], poseOverride->m_outputPose[ outputIndex ], weight );
						}
#else
						Float weight = m_bones[i].m_weight;
						if ( weight > 0.f )
						{
							output.m_outputPose[ outputIndex ].Lerp( output.m_outputPose[ outputIndex ], poseOverride->m_outputPose[ outputIndex ], weight );
						}
#endif
					}	
				}

				if ( m_overrideFloatTracks )
				{
					for ( Uint32 i=0; i<output.m_numFloatTracks && i<poseOverride->m_numFloatTracks; i++ )
					{
						output.m_floatTracks[i] = ::Max<Float>( output.m_floatTracks[i], poseOverride->m_floatTracks[i] );
					}
				}

				// Copy delta motion if requested
				if ( m_overrideDeltaMotion )
				{
					output.m_deltaReferenceFrameLocal = poseOverride->m_deltaReferenceFrameLocal;
				}

				if ( m_overrideCustomTracks )
				{
					COMPILE_ASSERT( SBehaviorGraphOutput::NUM_CUSTOM_FLOAT_TRACKS == 5 );
					output.m_customFloatTracks[0] = ::Max<Float>( output.m_customFloatTracks[0], poseOverride->m_customFloatTracks[0] );
					output.m_customFloatTracks[1] = ::Max<Float>( output.m_customFloatTracks[1], poseOverride->m_customFloatTracks[1] );
					output.m_customFloatTracks[2] = ::Max<Float>( output.m_customFloatTracks[2], poseOverride->m_customFloatTracks[2] );
					output.m_customFloatTracks[3] = ::Max<Float>( output.m_customFloatTracks[3], poseOverride->m_customFloatTracks[3] );
					output.m_customFloatTracks[4] = ::Max<Float>( output.m_customFloatTracks[4], poseOverride->m_customFloatTracks[4] );
				}

				if ( m_mergeEvents )
				{
					output.MergeEventsAsOverlays( *poseOverride, 1.f );
				}
				output.MergeUsedAnimsAsOverlays( *poseOverride, 1.f );
			}
		}
		else
		{
			// I am not that sure whether it should be current state or node's input
			currentState->Sample( context, instance, output );
		}
	}
}

Bool CBehaviorGraphSelfActivatingOverrideStateMachineNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool ret = TBaseClass::ProcessEvent( instance, event );

	if ( m_cachedInputNode && m_cachedInputNode != GetCurrentState( instance ) )
	{
		ret |= m_cachedInputNode->ProcessEvent( instance, event );
	}

	return ret;
}

Bool CBehaviorGraphSelfActivatingOverrideStateMachineNode::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool ret = TBaseClass::ProcessForceEvent( instance, event );

	if ( m_cachedInputNode && m_cachedInputNode != GetCurrentState( instance ) )
	{
		ret |= m_cachedInputNode->ProcessForceEvent( instance, event );
	}

	return ret;
}

void CBehaviorGraphSelfActivatingOverrideStateMachineNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphSelfActivatingOverrideStateMachineNode::OnActivated( CBehaviorGraphInstance& instance ) const
{	
	TBaseClass::OnActivated( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}
}

void CBehaviorGraphSelfActivatingOverrideStateMachineNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphSyncOverrideStateMachineNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphSyncOverrideStateMachineNode::GetCaption() const
{
	if ( !m_name.Empty() )
	{
		return String::Printf( TXT("Self Act State machine with sync override [ %s ]"), m_name.AsChar() );
	}
	else
	{
		return String( TXT("Self Act State machine with sync override") );
	}
}

#endif

void CBehaviorGraphSyncOverrideStateMachineNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_bones;
	compiler << i_boneRoot;
}

void CBehaviorGraphSyncOverrideStateMachineNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	Int32 rootBone = FindBoneIndex( m_rootBoneName, instance );
	if ( rootBone != -1 )
	{
		FillBones( rootBone, instance[ i_bones ], instance );
	}

	instance[ i_boneRoot ] = rootBone;
}

void CBehaviorGraphSyncOverrideStateMachineNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_boneRoot );
	INST_PROP( i_bones );
}

void CBehaviorGraphSyncOverrideStateMachineNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	BEH_NODE_UPDATE( SyncOverrideStateMachine );
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphSyncOverrideStateMachineNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	if ( !m_cachedInputNode )
	{
		return;
	}

	CBehaviorGraphNode* currentState = GetCurrentState( instance );
	if ( currentState ) 
	{
		ASSERT( currentState->IsActive( instance ) );

		if ( IsRunning( instance ) )
		{
			ASSERT( m_cachedInputNode->IsActive( instance ) );

			// Get pose
			CCacheBehaviorGraphOutput cachePose( context );
			SBehaviorGraphOutput* poseOverride = cachePose.GetPose();
			if ( poseOverride )
			{
				ASSERT( currentState->IsActive( instance ) );

				m_cachedInputNode->Sample( context, instance, output );
				currentState->Sample( context, instance, *poseOverride );

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
				if ( m_mergeEvents )
				{
					output.MergeEvents( *poseOverride, 1.f );
				}
				output.MergeUsedAnimsAsOverlays( *poseOverride, 1.f );
			}
		}
		else
		{
			currentState->Sample( context, instance, output );
		}
	}
}

Bool CBehaviorGraphSyncOverrideStateMachineNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool ret = TBaseClass::ProcessEvent( instance, event );

	if ( m_cachedInputNode && m_cachedInputNode != GetCurrentState( instance ) )
	{
		ret |= m_cachedInputNode->ProcessEvent( instance, event );
	}

	return ret;
}

Bool CBehaviorGraphSyncOverrideStateMachineNode::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool ret = TBaseClass::ProcessForceEvent( instance, event );

	if ( m_cachedInputNode && m_cachedInputNode != GetCurrentState( instance ) )
	{
		ret |= m_cachedInputNode->ProcessForceEvent( instance, event );
	}

	return ret;
}

void CBehaviorGraphSyncOverrideStateMachineNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphSyncOverrideStateMachineNode::OnActivated( CBehaviorGraphInstance& instance ) const
{	
	TBaseClass::OnActivated( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}
}

void CBehaviorGraphSyncOverrideStateMachineNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}
}

void CBehaviorGraphSyncOverrideStateMachineNode::ConnectTwoPoses( SBehaviorGraphOutput &a, SBehaviorGraphOutput &b, Uint32 bone, CBehaviorGraphInstance& instance ) const
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
#ifdef USE_HAVOK_ANIMATION
		hkQsTransform boneParentAMS = a.GetBoneModelTransform( parent, skeleton->GetParentIndices() );
		hkQsTransform boneBMS = b.GetBoneModelTransform( bone, skeleton->GetParentIndices() );

		hkQsTransform final; final.setMulInverseMul( boneParentAMS, boneBMS );

		// Write final bone rotation to pose 'a'
		a.m_outputPose[ bone ].m_rotation = final.m_rotation;
#else
		RedQsTransform boneParentAMS = a.GetBoneModelTransform( parent, skeleton->GetParentIndices() );
		RedQsTransform boneBMS = b.GetBoneModelTransform( bone, skeleton->GetParentIndices() );

		RedQsTransform final; 
		final.SetMulInverseMul( boneParentAMS, boneBMS );

		// Write final bone rotation to pose 'a'
		a.m_outputPose[ bone ].Rotation = final.Rotation;
#endif
	}
}

void CBehaviorGraphSyncOverrideStateMachineNode::FillBones( Int32 rootBone, TDynArray< Int32 >& bones, CBehaviorGraphInstance& instance ) const
{
	for ( BoneChildrenIterator it( instance.GetAnimatedComponent(), rootBone ); it; ++it )
	{
		bones.PushBack( *it );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphDefaultSelfActAdditiveStateNode );

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool CBehaviorGraphDefaultSelfActAdditiveStateNode::CanBeExpanded() const
{
	return false;
}

#endif

void CBehaviorGraphDefaultSelfActAdditiveStateNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	CBehaviorGraphSelfActivatingAdditiveStateMachineNode* machine = SafeCast< CBehaviorGraphSelfActivatingAdditiveStateMachineNode >( GetParentNode() );
	m_rootNode = machine->GetInputNode();
}

void CBehaviorGraphDefaultSelfActAdditiveStateNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	ASSERT( 0 );
}
void CBehaviorGraphDefaultSelfActAdditiveStateNode:: SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	ASSERT( 0 );
}

Bool CBehaviorGraphDefaultSelfActAdditiveStateNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	return false;
}

Bool CBehaviorGraphDefaultSelfActAdditiveStateNode::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStateAdditiveTransitionNode );

void CBehaviorGraphStateAdditiveTransitionNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	if ( m_cachedStartStateNode && m_cachedEndStateNode )
	{
		RED_WARNING( m_cachedStartStateNode->IsA< CBehaviorGraphDefaultSelfActAdditiveStateNode >() ||
					 m_cachedEndStateNode->IsA< CBehaviorGraphDefaultSelfActAdditiveStateNode >(), "This transition should be used for transitioning to or from additive state node. It's in '%ls'", GetGraph()->GetDepotPath().AsChar() );

		m_cachedAdditiveNode = m_cachedStartStateNode->IsA< CBehaviorGraphDefaultSelfActAdditiveStateNode >() ? m_cachedEndStateNode : m_cachedStartStateNode;
	}
}

void CBehaviorGraphStateAdditiveTransitionNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	BEH_NODE_SAMPLE( TransitionAdditiveBlend );

	const Float alpha = GetAlpha( instance );

	if ( m_cachedAdditiveNode ) 
	{
		m_cachedAdditiveNode->Sample( context, instance, output );
#ifdef USE_HAVOK_ANIMATION
		const hkQsTransform ident( hkQsTransform::IDENTITY );

		const Uint32 size = output.m_numBones;
		for ( Uint32 i=0; i<size; ++i )
		{
			hkQsTransform& bone = output.m_outputPose[ i ];

			bone.setInterpolate4( ident, bone, alpha );
		}
#else
		const RedQsTransform ident( RedQsTransform::IDENTITY );

		const Uint32 size = output.m_numBones;
		for ( Uint32 i = 0; i < size; ++i )
		{
			RedQsTransform& bone = output.m_outputPose[ i ];

			bone.Lerp( ident, bone, alpha );
		}
#endif
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphSelfActivatingAdditiveStateMachineNode );

CBehaviorGraphSelfActivatingAdditiveStateMachineNode::CBehaviorGraphSelfActivatingAdditiveStateMachineNode()
	: m_type( AT_Ref )
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphSelfActivatingAdditiveStateMachineNode::GetCaption() const
{
	if ( !m_name.Empty() )
	{
		return String::Printf( TXT("Self Act Addtive State machine [ %s ]"), m_name.AsChar() );
	}
	else
	{
		return String( TXT("Self Act Addtive State machine") );
	}
}

CBehaviorGraphStateNode* CBehaviorGraphSelfActivatingAdditiveStateMachineNode::CreateDefaultStateNode()
{
	GraphBlockSpawnInfo spawnInfo( CBehaviorGraphDefaultSelfActAdditiveStateNode::GetStaticClass() );

	CBehaviorGraphDefaultSelfActAdditiveStateNode* newNode = SafeCast< CBehaviorGraphDefaultSelfActAdditiveStateNode >( CreateChildNode( spawnInfo.GetClass() ) );

	return newNode;
}

#endif

void CBehaviorGraphSelfActivatingAdditiveStateMachineNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	BEH_NODE_UPDATE( SelfActivatingAdditiveStateMachine );
	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphSelfActivatingAdditiveStateMachineNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	if ( !m_cachedInputNode )
	{
		return;
	}

	CBehaviorGraphNode* currentState = GetCurrentState( instance );
	if ( currentState ) 
	{
		ASSERT( currentState->IsActive( instance ) );

		if ( IsRunning( instance ) )
		{
			ASSERT( m_cachedInputNode->IsActive( instance ) );

			if ( currentState != m_defaultNode )
			{
				CCacheBehaviorGraphOutput cachePose( context );
				SBehaviorGraphOutput* poseAdditive = cachePose.GetPose();

				if ( poseAdditive )
				{
					poseAdditive->SetIdentity();

					m_cachedInputNode->Sample( context, instance, output );
					currentState->Sample( context, instance, *poseAdditive );

					const Uint32 size = output.m_numBones;

					if ( m_type == AT_Ref )
					{
						for ( Uint32 i=0; i<size;  ++i )
						{
#ifdef USE_HAVOK_ANIMATION
							output.m_outputPose[ i ].setMul( poseAdditive->m_outputPose[ i ], output.m_outputPose[ i ] );
#else
							output.m_outputPose[ i ].SetMul( poseAdditive->m_outputPose[ i ], output.m_outputPose[ i ] );
#endif
						}
					}
					else if ( m_type == AT_Local )
					{
						for ( Uint32 i=0; i<size;  ++i )
						{
#ifdef USE_HAVOK_ANIMATION
							output.m_outputPose[ i ].setMul( output.m_outputPose[ i ], poseAdditive->m_outputPose[ i ] );
#else
							output.m_outputPose[ i ].SetMul( output.m_outputPose[ i ], poseAdditive->m_outputPose[ i ] );
#endif
						}
					}

					if ( m_mergeEvents )
					{
						output.MergeEvents( *poseAdditive, 1.f );
					}
					output.MergeUsedAnimsAsAdditives( *poseAdditive, 1.f );
				}
			}
			else
			{
				m_cachedInputNode->Sample( context, instance, output );
			}
		}
		else
		{
			currentState->Sample( context, instance, output );
		}
	}
}

Bool CBehaviorGraphSelfActivatingAdditiveStateMachineNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool ret = TBaseClass::ProcessEvent( instance, event );

	if ( m_cachedInputNode && m_cachedInputNode != GetCurrentState( instance ) )
	{
		ret |= m_cachedInputNode->ProcessEvent( instance, event );
	}

	return ret;
}

Bool CBehaviorGraphSelfActivatingAdditiveStateMachineNode::ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool ret = TBaseClass::ProcessForceEvent( instance, event );

	if ( m_cachedInputNode && m_cachedInputNode != GetCurrentState( instance ) )
	{
		ret |= m_cachedInputNode->ProcessForceEvent( instance, event );
	}

	return ret;
}

void CBehaviorGraphSelfActivatingAdditiveStateMachineNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphSelfActivatingAdditiveStateMachineNode::OnActivated( CBehaviorGraphInstance& instance ) const
{	
	TBaseClass::OnActivated( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}
}

void CBehaviorGraphSelfActivatingAdditiveStateMachineNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
