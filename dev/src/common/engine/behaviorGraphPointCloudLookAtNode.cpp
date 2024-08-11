/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorGraphPointCloudLookAtNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "../engine/animPointCloudLookAtParam.h"
#include "cacheBehaviorGraphOutput.h"
#include "behaviorGraphUtils.inl"
#include "allocatedBehaviorGraphOutput.h"
#include "../engine/havokAnimationUtils.h"
#include "../engine/entity.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/curve.h"
#include "../engine/skeletalAnimationContainer.h"
#include "../engine/graphConnectionRebuilder.h"
#include "animatedComponent.h"
#include "behaviorProfiler.h"
#include "behaviorIncludes.h"
#include "Behavior/Tools/bitTools.h"
#include "pose.h"
#include "skeleton.h"
#include "../core/mathUtils.h"
#include "controlRigAlgorithms.h"

RED_DEFINE_STATIC_NAME( Progress );
RED_DEFINE_STATIC_NAME( UseSecA );
RED_DEFINE_STATIC_NAME( UseSecB );

using namespace BehaviorGraphTools;

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

//#define DEBUG_POINT_CLOUD_LOOK_AT

#ifdef DEBUG_POINT_CLOUD_LOOK_AT
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( IBehaviorGraphPointCloudLookAtTransition );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphPointCloudLookAtNode );

CBehaviorGraphPointCloudLookAtNode::CBehaviorGraphPointCloudLookAtNode()
	: m_additiveMode( false )
	, m_additiveType( AT_Local )
	, m_writeToPoseLikeAdditiveNode( false )
	, m_convertAnimationToAdditiveFlagA( false )
	, m_convertAnimationToAdditiveRefFrameNumA( 0 )
	, m_convertAnimationToAdditiveFlagB( false )
	, m_convertAnimationToAdditiveRefFrameNumB( 0 )
	, m_useTransitionWeightPred( true )
	, m_useBlendInsteadOfTargetTransition( true )
{

}

void CBehaviorGraphPointCloudLookAtNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_progress;
	compiler << i_weightA;
	compiler << i_weightB;
	compiler << i_useSecBlendCloudA;
	compiler << i_useSecBlendCloudB;
	compiler << i_lowerBodyPartsWeight;
	compiler << i_lowerBodyPartBones;
	compiler << i_targetA;
	compiler << i_targetB;
	compiler << i_targetSetA;
	compiler << i_targetSetB;
	compiler << i_animationA;
	compiler << i_animationB;
	compiler << i_boneIndex;
	compiler << i_paramsA;
	compiler << i_paramsB;
	compiler << i_cookieA;
	compiler << i_cookieB;
	compiler << i_cachedPoseForAdditiveModeA;
	compiler << i_cachedPoseForAdditiveModeB;
	compiler << i_transition;
	compiler << i_transitionBonesPred;
	compiler << i_waitingForAnimationFullyLoad;
	compiler << i_handL;
	compiler << i_handR;
	compiler << i_duration;
	compiler << i_useDeformationMS;

#ifndef NO_EDITOR
	compiler << i_lastBoneWS_Pre;
	compiler << i_lastBoneWS_Post;
	compiler << i_lastBoneDir;
	compiler << i_lastPointPosOnSphere;
	compiler << i_lastTri;
	compiler << i_lastWeights;
	compiler << i_lastNNpoint;
	compiler << i_handPoints;
#endif

	if ( m_transition )
	{
		m_transition->OnBuildDataLayout( compiler );
	}

	if ( m_secondaryMotion )
	{
		m_secondaryMotion->OnBuildDataLayout( compiler );
	}
}

void CBehaviorGraphPointCloudLookAtNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );

	instance[ i_progress ] = 0.f;
	instance[ i_weightA ] = 0.f;
	instance[ i_weightB ] = 0.f;
	instance[ i_useSecBlendCloudA ] = false;
	instance[ i_useSecBlendCloudB ] = false;
	instance[ i_lowerBodyPartsWeight ] = 1.f;
	instance[ i_targetA ] = Vector::ZERO_3D_POINT;
	instance[ i_targetB ] = Vector::ZERO_3D_POINT;
	instance[ i_targetSetA ] = false;
	instance[ i_targetSetB ] = false;
	instance[ i_cookieA ] = AnimPointCloud::SphereLookAt::GenerateCookie();
	instance[ i_cookieB ] = AnimPointCloud::SphereLookAt::GenerateCookie();
	instance[ i_boneIndex ] = FindBoneIndex( m_boneName, instance );
	instance[ i_handL ] = FindBoneIndex( m_handL, instance );
	instance[ i_handR ] = FindBoneIndex( m_handR, instance );
	instance[ i_duration ] = 0.f;
	instance[ i_useDeformationMS ] = true;

	BitTools::ClearMask( instance[ i_waitingForAnimationFullyLoad ] );

	const Uint32 lowerBodyPartBonesSize = m_lowerBodyPartBones.Size();
	TDynArray< Int32 >& lowerBodyPartBones = instance[ i_lowerBodyPartBones ];
	lowerBodyPartBones.Reserve( lowerBodyPartBonesSize );
	for ( Uint32 i=0; i<lowerBodyPartBonesSize; ++i )
	{
		const SBehaviorGraphBoneInfo& b = m_lowerBodyPartBones[ i ];
		if ( !b.m_boneName.Empty() )
		{
			const Int32 boneIdx = FindBoneIndex( b.m_boneName, instance );
			if ( boneIdx != -1 )
			{
				lowerBodyPartBones.PushBack( boneIdx );
			}
		}
	}

#ifndef NO_EDITOR
	instance[ i_lastBoneWS_Pre ].Resize( 2 );
	instance[ i_lastBoneWS_Post ].Resize( 2 );
	instance[ i_lastBoneDir ].Resize( 2 );
	instance[ i_lastPointPosOnSphere ].Resize( 2 );
	instance[ i_lastNNpoint ].Resize( 2 );
	instance[ i_lastTri ].Resize( 2 );
	instance[ i_lastWeights ].Resize( 2 );
	instance[ i_handPoints ].Resize( 12 );

	instance[ i_lastBoneWS_Pre ][ Pose_A ] = instance[ i_lastBoneWS_Pre ][ Pose_B ] = Matrix::IDENTITY;
	instance[ i_lastBoneWS_Post ][ Pose_A ] = instance[ i_lastBoneWS_Post ][ Pose_B ] = Matrix::IDENTITY;
	instance[ i_lastBoneDir ][ Pose_A ] = instance[ i_lastBoneDir ][ Pose_B ] = Vector::EX;
	instance[ i_lastPointPosOnSphere ][ Pose_A ] = instance[ i_lastPointPosOnSphere ][ Pose_B ] = Vector::ZERO_3D_POINT;
	instance[ i_lastNNpoint ][ Pose_A ] = instance[ i_lastNNpoint ][ Pose_B ] = -1;

	TDynArray< Int32 >& lastTriA = instance[ i_lastTri ][ Pose_A ];
	TDynArray< Float >& lastWeightA = instance[ i_lastWeights ][ Pose_A ];
	TDynArray< Int32 >& lastTriB = instance[ i_lastTri ][ Pose_B ];
	TDynArray< Float >& lastWeightB = instance[ i_lastWeights ][ Pose_B ];
	lastTriA.Resize( 3 );
	lastWeightA.Resize( 3 );
	lastTriB.Resize( 3 );
	lastWeightB.Resize( 3 );
	for ( Uint32 i=0; i<3; ++i )
	{
		lastTriA[ i ] = lastTriB[ i ] = -1;
		lastWeightA[ i ] = lastWeightB[ i ] = 0.f;
	}
#endif

	RefreshBothAnimations( instance, m_animationA, m_animationB );

	instance[ i_transition ] = nullptr;
	TDynArray< Int32 >& transitionBonesPred = instance[ i_transitionBonesPred ];
	{
		const Uint32 numTransBones = m_transitionBonesPred.Size();
		transitionBonesPred.Reserve( numTransBones );
		for ( Uint32 i=0; i<numTransBones; ++i )
		{
			const Int32 bone = FindBoneIndex( m_transitionBonesPred[ i ], instance );
			if ( bone != -1 )
			{
				transitionBonesPred.PushBack( bone );
			}
		}
	}

	if ( m_transition )
	{
		m_transition->OnInitInstance( instance );

		instance[ i_transition ] = m_transition;
	}

	if ( m_secondaryMotion )
	{
		m_secondaryMotion->OnInitInstance( instance );
	}
}

void CBehaviorGraphPointCloudLookAtNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	/*if ( m_secondaryMotion )
	{
		m_secondaryMotion->OnReleaseInstance( instance );
	}*/

	if ( m_transition )
	{
		m_transition->OnReleaseInstance( instance );
	}

	ASSERT( !instance[ i_cachedPoseForAdditiveModeA ].HasPose() );
	ASSERT( !instance[ i_cachedPoseForAdditiveModeB ].HasPose() );

	RefreshBothAnimations( instance, CName::NONE, CName::NONE );

	ASSERT( !instance[ i_animationA ] );
	ASSERT( !instance[ i_animationB ] );

	TBaseClass::OnReleaseInstance( instance );
}

void CBehaviorGraphPointCloudLookAtNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_progress );
	INST_PROP( i_weightA );
	INST_PROP( i_weightB );
	INST_PROP( i_useSecBlendCloudA );
	INST_PROP( i_useSecBlendCloudB );
	INST_PROP( i_lowerBodyPartsWeight );
	INST_PROP( i_targetA );
	INST_PROP( i_targetB );
	INST_PROP( i_targetSetA );
	INST_PROP( i_targetSetB );
	INST_PROP( i_boneIndex );
	INST_PROP( i_cookieA );
	INST_PROP( i_cookieB );
	INST_PROP( i_paramsA );
	INST_PROP( i_paramsB );
	INST_PROP( i_animationA );
	INST_PROP( i_animationB );
	INST_PROP( i_transitionBonesPred );
	INST_PROP( i_handL );
	INST_PROP( i_handR );
}

void CBehaviorGraphPointCloudLookAtNode::SetAnimations( CBehaviorGraphInstance& instance, const CName& animationA, const CName& animationB ) const
{
	const CSkeletalAnimationSetEntry* animA = instance[ i_animationA ];
	const CSkeletalAnimationSetEntry* animB = instance[ i_animationB ];

	const CName prevAnimationA = animA ? animA->GetName() : CName::NONE;
	const CName prevAnimationB = animB ? animB->GetName() : CName::NONE;

	if ( prevAnimationA != animationA )
	{
		RefreshSingleAnimation( instance, animationA, Pose_A );

		if ( IsActive( instance ) )
		{
			FreePose( instance, Pose_A );
			TryCachePoseForAdditiveMode( instance, Pose_A );	
		}
	}

	if ( prevAnimationB != animationB )
	{
		RefreshSingleAnimation( instance, animationB, Pose_B );

		if ( IsActive( instance ) )
		{
			FreePose( instance, Pose_B );
			TryCachePoseForAdditiveMode( instance, Pose_B );
		}
	}
}

void CBehaviorGraphPointCloudLookAtNode::RefreshBothAnimations( CBehaviorGraphInstance& instance, const CName& animationNameA, const CName& animationNameB ) const
{
	RefreshSingleAnimation( instance, animationNameA, Pose_A );
	RefreshSingleAnimation( instance, animationNameB, Pose_B );
}

void CBehaviorGraphPointCloudLookAtNode::RefreshSingleAnimation( CBehaviorGraphInstance& instance, const CName& animationName, EPoseNum poseNum ) const
{
	const TInstanceVar< CSkeletalAnimationSetEntry* >& i_anim = poseNum == Pose_A ? i_animationA : i_animationB;
	const TInstanceVar< CAnimPointCloudLookAtParam* >& i_param = poseNum == Pose_A ? i_paramsA : i_paramsB;
	
	ReleaseAnimationUsage( instance[ i_anim ] );

	instance[ i_anim ] = NULL;
	instance[ i_param ] = NULL;

	CSkeletalAnimationSetEntry* animation = animationName != CName::NONE ? FindAnimation( instance, animationName ) : nullptr;
	if ( animation )
	{
		instance[ i_anim ] = animation;
		const CAnimPointCloudLookAtParam* params = animation->FindParam< CAnimPointCloudLookAtParam >();
		instance[ i_param ] = const_cast< CAnimPointCloudLookAtParam* >( params ); // We can not use const with TInstanceVar
	}

	AddAnimationUsage( animation );
}

CSkeletalAnimationSetEntry* CBehaviorGraphPointCloudLookAtNode::FindAnimation( CBehaviorGraphInstance& instance, const CName& animationName ) const
{
	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	return ac->GetAnimationContainer() ? ac->GetAnimationContainer()->FindAnimation( animationName ) : NULL;
}

void CBehaviorGraphPointCloudLookAtNode::AddAnimationUsage( CSkeletalAnimationSetEntry* animEntry ) const
{
	if ( animEntry && animEntry->GetAnimation() )
	{
		animEntry->GetAnimation()->AddUsage();
	}
}

void CBehaviorGraphPointCloudLookAtNode::ReleaseAnimationUsage( CSkeletalAnimationSetEntry* animEntry ) const
{
	if ( animEntry && animEntry->GetAnimation() )
	{
		animEntry->GetAnimation()->ReleaseUsage();
	}
}

void CBehaviorGraphPointCloudLookAtNode::SampleAnimation( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Int32 pose, EPoseNum poseNum ) const
{
	const CSkeletalAnimationSetEntry* animation = poseNum == Pose_A ? instance[ i_animationA ] : instance[ i_animationB ];
	if ( animation )
	{
		ASSERT( animation->GetAnimation()->IsFullyLoaded() );

		animation->GetAnimation()->Sample( (Float)pose, output.m_numBones, output.m_numFloatTracks, output.m_outputPose, output.m_floatTracks );
		if ( instance.GetAnimatedComponent()->UseExtractedTrajectory() && instance.GetAnimatedComponent()->GetTrajectoryBone() != -1 )
		{
			output.ExtractTrajectory( instance.GetAnimatedComponent() );
		}
	}
}

void CBehaviorGraphPointCloudLookAtNode::FreePose( CBehaviorGraphInstance& instance, EPoseNum poseNum ) const
{
	CAllocatedBehaviorGraphOutput& pose = poseNum == Pose_A ? instance[ i_cachedPoseForAdditiveModeA ] : instance[ i_cachedPoseForAdditiveModeB ];
	if ( pose.HasPose() )
	{
		ASSERT( m_additiveMode );
		ASSERT( poseNum == Pose_A ? m_convertAnimationToAdditiveFlagA : m_convertAnimationToAdditiveFlagB );

		pose.Free( instance );
	}
}

void CBehaviorGraphPointCloudLookAtNode::ConvertPoseToAdditive( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, EPoseNum poseNum ) const
{
	const CAllocatedBehaviorGraphOutput& cachedPose = poseNum == Pose_A ? instance[ i_cachedPoseForAdditiveModeA ] : instance[ i_cachedPoseForAdditiveModeB ];
	if ( cachedPose.HasPose() )
	{
		const SBehaviorGraphOutput* frame = cachedPose.GetPose();
		ImportAnimationUtils::ConvertToAdditiveFrame( frame->m_outputPose, output.m_outputPose, output.m_numBones, m_additiveType );
	}

	ASSERT( cachedPose.HasPose() );
}

Bool CBehaviorGraphPointCloudLookAtNode::TryCachePoseForAdditiveMode( CBehaviorGraphInstance& instance, EPoseNum poseNum ) const
{
	const CSkeletalAnimationSetEntry* animation = poseNum == Pose_A ? instance[ i_animationA ] : instance[ i_animationB ];

	ASSERT( animation != nullptr );
	if ( animation )
	{
		const Int32 convertAnimationToAdditiveRefFrameNum = poseNum == Pose_A ? m_convertAnimationToAdditiveRefFrameNumA : m_convertAnimationToAdditiveRefFrameNumB;

		ASSERT( convertAnimationToAdditiveRefFrameNum != -1 );
		const Float time = convertAnimationToAdditiveRefFrameNum >= 0 ? (Float)convertAnimationToAdditiveRefFrameNum : 0.f;

		// Check if animation is fully streamed in.
		if ( animation->GetAnimation()->IsFullyLoaded() )
		{
			// Animation is ready, so create a cache for pose:
			CAllocatedBehaviorGraphOutput& cachedPose = poseNum == Pose_A ? instance[ i_cachedPoseForAdditiveModeA ] : instance[ i_cachedPoseForAdditiveModeB ];

			if ( cachedPose.HasPose() )
			{
				ASSERT( !cachedPose.HasPose() );
				cachedPose.Free( instance );
			}

			cachedPose.Create( instance );
			SBehaviorGraphOutput* pose = cachedPose.GetPose();

			// Sample animation:
			animation->GetAnimation()->Sample( time, pose->m_numBones, pose->m_numFloatTracks, pose->m_outputPose, pose->m_floatTracks );
			if ( instance.GetAnimatedComponent()->GetTrajectoryBone() != -1 )
			{
				pose->ExtractTrajectory( instance.GetAnimatedComponent() );
			}

			// Mark that we are no longer waiting for this animation.
			BitTools::ClearFlag( instance[ i_waitingForAnimationFullyLoad ], BitTools::NumberToFlag< Uint32 >( poseNum ) );
			return true;
		}
	}

	// We are still waiting for animation to be fully streamed in.
	BitTools::SetFlag( instance[ i_waitingForAnimationFullyLoad ], BitTools::NumberToFlag< Uint32 >( poseNum ) );

	return false;
}

//////////////////////////////////////////////////////////////////////////
Bool CBehaviorGraphPointCloudLookAtNode::IsNodeReadyToSample( CBehaviorGraphInstance& instance ) const
{
	// Check if we are waiting for some animations to be fully streamed in:
	if ( instance[ i_waitingForAnimationFullyLoad ] > 0 )
	{
		for( Uint32 i = 0; i < EPoseNum::COUNT; ++i )
		{
			// Are we waiting for i-th animation?
			if ( BitTools::IsFlagSet( instance[ i_waitingForAnimationFullyLoad ], BitTools::NumberToFlag< Uint32 >( i ) ) )
			{
				// Poll, maybe animation is ready now.
				if ( !TryCachePoseForAdditiveMode( instance, static_cast< EPoseNum > ( i ) ) )
				{
					// We still have to wait.
					return false;
				}
			}
		}
	}

	// Everything is streamed in, node is ready for work.
	return true;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphPointCloudLookAtNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	TBaseClass::OnRebuildSockets();

	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Progress ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( WeightA ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( WeightB ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( TargetA ) ) );
	CreateSocket( CBehaviorGraphVectorVariableInputSocketSpawnInfo( CNAME( TargetB ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( UseSecA ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( UseSecB ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Duration ) ) );
	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Fallback ) ) );
}

void CBehaviorGraphPointCloudLookAtNode::OnSpawned(const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );

	if ( !m_transitionPredCurve )
	{
		m_transitionPredCurve = CreateObject< CCurve >( this );
		m_transitionPredCurve->AddPoint( 0.f, 0.f );
		m_transitionPredCurve->AddPoint( 1.f, 1.f );
	}

	if ( !m_targetWeightCurve )
	{
		m_targetWeightCurve = CreateObject< CCurve >( this );
		m_targetWeightCurve->AddPoint( 0.f, 0.f );
		m_targetWeightCurve->AddPoint( 1.f, 1.f );
	}

	if ( !m_targetWeightCurve2 )
	{
		m_targetWeightCurve2 = CreateObject< CCurve >( this );
		m_targetWeightCurve2->AddPoint( 0.f, 0.f );
		m_targetWeightCurve2->AddPoint( 1.f, 1.f );
	}

	if ( !m_headDownCurve )
	{
		m_headDownCurve = CreateObject< CCurve >( this );
		m_headDownCurve->AddPoint( 0.f, 0.f );
		m_headDownCurve->AddPoint( 1.f, 1.f );
	}

	if ( !m_headProgressCurve )
	{
		m_headProgressCurve = CreateObject< CCurve >( this );
		m_headProgressCurve->AddPoint( 0.f, 0.f );
		m_headProgressCurve->AddPoint( 1.f, 1.f );
	}

	if ( !m_handDragCurve )
	{
		m_handDragCurve = CreateObject< CCurve >( this );
		m_handDragCurve->AddPoint( 0.f, 0.f );
		m_handDragCurve->AddPoint( 1.f, 1.f );
	}
}

#endif

void CBehaviorGraphPointCloudLookAtNode::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	if ( !m_transitionPredCurve )
	{
		m_transitionPredCurve = CreateObject< CCurve >( this );
		m_transitionPredCurve->AddPoint( 0.f, 0.f );
		m_transitionPredCurve->AddPoint( 1.f, 1.f );
	}

	if ( !m_targetWeightCurve )
	{
		m_targetWeightCurve = CreateObject< CCurve >( this );
		m_targetWeightCurve->AddPoint( 0.f, 0.f );
		m_targetWeightCurve->AddPoint( 1.f, 1.f );
	}

	if ( !m_targetWeightCurve2 )
	{
		m_targetWeightCurve2 = CreateObject< CCurve >( this );
		m_targetWeightCurve2->AddPoint( 0.f, 0.f );
		m_targetWeightCurve2->AddPoint( 1.f, 1.f );
	}

	if ( !m_headDownCurve )
	{
		m_headDownCurve = CreateObject< CCurve >( this );
		m_headDownCurve->AddPoint( 0.f, 0.f );
		m_headDownCurve->AddPoint( 1.f, 1.f );
	}

	if ( !m_headProgressCurve )
	{
		m_headProgressCurve = CreateObject< CCurve >( this );
		m_headProgressCurve->AddPoint( 0.f, 0.f );
		m_headProgressCurve->AddPoint( 1.f, 1.f );
	}

	if ( !m_handDragCurve )
	{
		m_handDragCurve = CreateObject< CCurve >( this );
		m_handDragCurve->AddPoint( 0.f, 0.f );
		m_handDragCurve->AddPoint( 1.f, 1.f );
	}
}

void CBehaviorGraphPointCloudLookAtNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( PointCloud );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	if ( m_cachedProgressNode )
	{
		m_cachedProgressNode->Update( context, instance, timeDelta );
		instance[ i_progress ] = m_cachedProgressNode->GetValue( instance );
	}

	if ( m_cachedWeightANode )
	{
		m_cachedWeightANode->Update( context, instance, timeDelta );
		instance[ i_weightA ] = m_cachedWeightANode->GetValue( instance );
	}

	if ( m_cachedWeightBNode )
	{
		m_cachedWeightBNode->Update( context, instance, timeDelta );
		instance[ i_weightB ] = m_cachedWeightBNode->GetValue( instance );
	}

	if ( m_cachedUseSecBlendANode )
	{
		m_cachedUseSecBlendANode->Update( context, instance, timeDelta );
		instance[ i_useSecBlendCloudA ] = m_cachedUseSecBlendANode->GetValue( instance ) > 0.5f;
	}

	if ( m_cachedUseSecBlendBNode )
	{
		m_cachedUseSecBlendBNode->Update( context, instance, timeDelta );
		instance[ i_useSecBlendCloudB ] = m_cachedUseSecBlendBNode->GetValue( instance ) > 0.5f;
	}

	if ( m_cachedDurationNode )
	{
		m_cachedDurationNode->Update( context, instance, timeDelta );
		instance[ i_duration ] = m_cachedDurationNode->GetValue( instance );
	}

	if ( m_cachedTargetNodeA )
	{
		m_cachedTargetNodeA->Update( context, instance, timeDelta );
		SetTargetA( instance, m_cachedTargetNodeA->GetVectorValue( instance ) );
	}

	if ( m_cachedTargetNodeB )
	{
		m_cachedTargetNodeB->Update( context, instance, timeDelta );
		SetTargetB( instance, m_cachedTargetNodeB->GetVectorValue( instance ) );
	}

	if ( m_cachedFallbackNode )
	{
		m_cachedFallbackNode->Update( context, instance, timeDelta );
	}

	IBehaviorGraphPointCloudLookAtTransition* transition = instance[ i_transition ];
	if ( transition )
	{
		transition->Update( instance, timeDelta );
	}
}

void CBehaviorGraphPointCloudLookAtNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( PointCloud );

	TBaseClass::Sample( context, instance, output );

	const Float progress = instance[ i_progress ];
	const Int32 boneIndex = instance[ i_boneIndex ];

	if ( progress > 0.f && 
		boneIndex != -1 && 
		IsNodeReadyToSample( instance ) ) // Animation is not ready, so pass simply pass the signal, cuz the node is not ready to work.
	{
		const Bool isTargetSetA = instance[ i_targetSetA ];
		const Bool isTargetSetB = instance[ i_targetSetB ];

		const Bool isTransitionBetweenTargets = isTargetSetA && isTargetSetB;
		const Bool isTransitionFromZero = isTargetSetB && !isTargetSetA;
		const Bool isTransitionToZero = !isTargetSetB;
		ASSERT( isTransitionBetweenTargets || isTransitionFromZero || isTransitionToZero );

		if ( m_useTransitionWeightPred && m_writeToPoseLikeAdditiveNode && ( isTransitionFromZero || isTransitionBetweenTargets || isTransitionToZero ) )
		{
			const Float progressBezier = BehaviorUtils::BezierInterpolation( progress );
			const Float progressPred = m_transitionPredCurve->GetFloatValue( progress );
			const Float progressTarget = MapTargetWeight( instance, progress );

			const Float weightA = instance[ i_weightA ];
			const Float weightB = instance[ i_weightB ];

			const Bool useSecBlendCloudA = instance[ i_useSecBlendCloudA ];
			const Bool useSecBlendCloudB = instance[ i_useSecBlendCloudB ];

			const AnimQsTransform boneMS = output.GetBoneModelTransform( instance.GetAnimatedComponent(), boneIndex );
			const AnimQsTransform l2w = MatrixToAnimQsTransform( instance.GetAnimatedComponent()->GetThisFrameTempLocalToWorld() );

			const Float zeroEps = NumericLimits< Float >::Epsilon();
			const Float oneEps = 1.f - zeroEps;

			//const Bool useOnlyBaseCloud = blendWeight <= zeroEps; // we can not use transition with two cloud target because we fake and use sec cloud to add to first one

			Bool useOnlyBaseCloud( false );
			Float blendWeight( 0.f );
			Float sampleWeight( 0.f );
			if ( isTransitionBetweenTargets )
			{
				if ( useSecBlendCloudA && useSecBlendCloudB )
				{
					blendWeight = 1.f;
				}
				else if ( useSecBlendCloudB )
				{
					blendWeight = progressBezier;
				}
				else if ( useSecBlendCloudA )
				{
					blendWeight = 1.f - progressBezier;
				}

				sampleWeight = Lerp( /*progress*/ progressTarget, weightA, weightB ); // I use progressTarget instead of progress because visual result is better
				useOnlyBaseCloud = !useSecBlendCloudA && !useSecBlendCloudB;
			}
			else if ( isTransitionFromZero )
			{
				ASSERT( isTargetSetB );
				blendWeight = useSecBlendCloudB ? 1.f : 0.f;
				sampleWeight = weightB;
				useOnlyBaseCloud = !useSecBlendCloudB;
			}
			else // isTransitionToZero
			{
				ASSERT( isTargetSetA );
				blendWeight = useSecBlendCloudA ? 1.f : 0.f;
				sampleWeight = weightA;
				useOnlyBaseCloud = !useSecBlendCloudA;
			}

			const Vector& vPointB = instance[ i_targetB ];

			CCacheBehaviorGraphOutput cachePoseB( context );
			SBehaviorGraphOutput* poseB = cachePoseB.GetPose();
			if ( poseB )
			{
				*poseB = output;

				if ( m_useBlendInsteadOfTargetTransition )
				{
					CCacheBehaviorGraphOutput cachePoseC( context );
					SBehaviorGraphOutput* poseC = cachePoseC.GetPose();
					if ( poseC )
					{
						if ( isTransitionFromZero )
						{
							output.SetIdentity();
						}
						else // isTransitionBetweenTargets or isTransitionToZero
						{
							ASSERT( isTargetSetA );
							const Vector& vPointA = instance[ i_targetA ]; // this is valid only for isTransitionBetweenTargets

							SampleInternal( context, instance, output, boneMS, l2w, sampleWeight, vPointA, blendWeight );
						}
						
						if ( !isTransitionToZero )  // isTransitionBetweenTargets or isTransitionFromZero
						{
							ASSERT( isTargetSetB );
							SampleInternal( context, instance, *poseC, boneMS, l2w, sampleWeight, vPointB, blendWeight );
						}
						else // isTransitionToZero
						{
							poseC->SetIdentity();
						}

						static Bool CAN_USE_DEF_MS = false;
						Bool USE_OLD_CODE = !(CAN_USE_DEF_MS && instance[i_useDeformationMS] );

						//if ( m_secondaryMotion )
						//{
						//	m_secondaryMotion->SampleTransitionPre( context, instance, output, *poseC, progress );
						//}

						if ( USE_OLD_CODE )
						{
							output.SetInterpolate( output, *poseC, progressTarget );
						}
						else if ( !(useOnlyBaseCloud && !isTransitionToZero) )
						{
							output.SetInterpolate( output, *poseC, progressTarget );
						}

						//if ( m_secondaryMotion )
						//{
						//	m_secondaryMotion->SampleTransitionPost( context, instance, output, progress );
						//}

						if ( useOnlyBaseCloud && !isTransitionToZero )
						{
							if ( USE_OLD_CODE )
							{
								//if ( m_writeToPoseLikeAdditiveNode )
								//{
								BehaviorUtils::BlendingUtils::BlendAdditive( *poseB, *poseB, output, 1.f, m_additiveType );
								const AnimQsTransform boneMS_Post = poseB->GetBoneModelTransform( instance.GetAnimatedComponent(), boneIndex );
								//}

								ASSERT( isTargetSetB );
								SampleInternal( context, instance, *poseB, boneMS_Post, l2w, sampleWeight, vPointB, 1.f );

								const TDynArray< Int32 >& transitionBonesPred = instance[ i_transitionBonesPred ];
								const Uint32 numTransBones = transitionBonesPred.Size();

								static Bool OVERRIDE_HEAD_F = false;
								static Float OVERRIDE_HEAD_W = 0.85f;

								Float w = Clamp( progressPred, 0.f, 1.f - progressTarget );

								static Bool USE_THIS = true;
								if ( USE_THIS )
								{
									w = Clamp( progressPred * ( 1.f - progressTarget ), 0.f, 1.f );
								}

								for ( Uint32 i=0; i<numTransBones; ++i )
								{
									const Int32 bone = transitionBonesPred[ i ];
									ASSERT( bone != -1 );

									AnimQsTransform temp;
									temp.SetMul( poseB->m_outputPose[ bone ], output.m_outputPose[ bone ] );

									if ( OVERRIDE_HEAD_F )
									{
										output.m_outputPose[ bone ].Lerp( output.m_outputPose[ bone ], temp, OVERRIDE_HEAD_W );
									}
									else
									{
										output.m_outputPose[ bone ].Lerp( output.m_outputPose[ bone ], temp, w );
									}
								}
							}
							else
							{
								CCacheBehaviorGraphOutput cachePoseD( context );
								CCacheBehaviorGraphOutput cachePoseE( context );
								SBehaviorGraphOutput* poseD = cachePoseD.GetPose();
								SBehaviorGraphOutput* poseE = cachePoseE.GetPose();

								SBehaviorGraphOutput* inputPose = poseB; // poz
								SBehaviorGraphOutput* cloudPoseA = &output; // add
								SBehaviorGraphOutput* cloudPoseB = poseC; //add

								const CSkeleton* sk = instance.GetAnimatedComponent()->GetSkeleton();
								const TDynArray< Int32 >& transitionBonesPred = instance[ i_transitionBonesPred ];
								const Uint32 numTransBones = transitionBonesPred.Size();
								if ( numTransBones > 0 && poseD && poseE )
								{
									const Int32 boneIdx = transitionBonesPred[0];
									const Int32 parentIdx = sk->GetParentBoneIndex( boneIdx );
									if ( parentIdx != -1 )
									{
										CPose pose( sk );

										const Float headProgress = progressTarget;
										const Float torsoProgress = Clamp<Float>(m_targetWeightCurve2->GetFloatValue(progress), 0.f, 1.f );

										poseD->SetInterpolate( *cloudPoseA, *cloudPoseB, headProgress ); // Add
										BehaviorUtils::BlendingUtils::BlendAdditive( *poseE, *inputPose, *poseD, 1.f, m_additiveType );

										pose.SetPoseLS( poseE->m_outputPose, poseE->m_numBones );
										
										AnimQsTransform boneA = pose.GetBoneMS(boneIdx);
										AnimQsTransform boneB = pose.GetBoneMS(boneIdx+1);

										const Float duration = instance[ i_duration ];
										const Int32 handL = instance[ i_handL ];
										const Int32 handR = instance[ i_handR ];
										const Bool useHandIk = false; //duration > 0.f && handL != -1 && handR != -1;
										AnimQsTransform boneMS_HandL;
										AnimQsTransform boneMS_HandR;
										if ( useHandIk )
										{
											const Float timeToMatch = duration * torsoProgress;
											const Float handDragValue = m_handDragCurve->GetFloatValue(timeToMatch);
											const Float timeToSet = Clamp( handDragValue, 0.f, duration );

											const Float handProgress = Clamp( timeToSet / duration, 0.f, 1.f );

											poseD->SetInterpolate( *cloudPoseA, *cloudPoseB, handProgress ); // Add
											BehaviorUtils::BlendingUtils::BlendAdditive( *poseE, *inputPose, *poseD, 1.f, m_additiveType );

											pose.SetPoseLS( poseE->m_outputPose, poseE->m_numBones );

											boneMS_HandL = pose.GetBoneMS(handL);
											boneMS_HandR = pose.GetBoneMS(handR);
										}

										poseD->SetInterpolate( *cloudPoseA, *cloudPoseB, torsoProgress );
										BehaviorUtils::BlendingUtils::BlendAdditive( *poseE, *inputPose, *poseD, 1.f, m_additiveType );

										pose.SetPoseLS( poseE->m_outputPose, poseE->m_numBones );

										pose.SetBoneMS(boneIdx+1,boneB, true);
										pose.SetBoneMS(boneIdx, boneA, true);

										SBehaviorGraphOutput* temp = pose.AccessSyncedPoseLS();
										output = *temp;

										{
											
											if ( useHandIk )
											{
												#ifndef NO_EDITOR
												TDynArray< Vector >& arr = instance[ i_handPoints ];
												#endif

												{
													const Int32 boneIndexC = handL;
													const Int32 boneIndexB = boneIndexC != -1 ? sk->GetParentBoneIndex( boneIndexC ) : -1;
													const Int32 boneIndexA = boneIndexB != -1 ? sk->GetParentBoneIndex( boneIndexB ) : -1;
													const Int32 boneIndexParentA = boneIndexA != -1 ? sk->GetParentBoneIndex( boneIndexA ) : -1;

													AnimQsTransform boneMS_ParentA = pose.GetBoneMS(boneIndexParentA);
													AnimQsTransform boneLS_A = output.m_outputPose[boneIndexA];
													AnimQsTransform boneLS_B = output.m_outputPose[boneIndexB];
													AnimQsTransform boneLS_C = output.m_outputPose[boneIndexC];

													#ifndef NO_EDITOR
													{
														AnimQsTransform boneMS_A; boneMS_A.SetMul(boneMS_ParentA, boneLS_A);
														AnimQsTransform boneMS_B; boneMS_B.SetMul(boneMS_A, boneLS_B);
														AnimQsTransform boneMS_C; boneMS_C.SetMul(boneMS_B, boneLS_C);

														arr[0] = AnimVectorToVector(boneMS_A.GetTranslation());
														arr[1] = AnimVectorToVector(boneMS_B.GetTranslation());
														arr[2] = AnimVectorToVector(boneMS_C.GetTranslation());
													}
													#endif

													Ik2Solver::Input sinput;
													sinput.m_firstJointParentTransformMS = boneMS_ParentA;
													sinput.m_firstJointLS = boneLS_A;
													sinput.m_secondJointLS = boneLS_B;
													sinput.m_endBoneLS = boneLS_C;
													sinput.m_hingeAxisLS = VectorToAnimVector(BehaviorUtils::VectorFromAxis(A_Y));
													sinput.m_endTargetMS = boneMS_HandL.GetTranslation();

													Ik2Solver::Output soutput;

													Ik2Solver::Solve(sinput, soutput);

													output.m_outputPose[boneIndexA] = soutput.m_firstJointLS;
													output.m_outputPose[boneIndexB] = soutput.m_secondJointLS;
													output.m_outputPose[boneIndexC] = soutput.m_endBoneLS;

													#ifndef NO_EDITOR
													{
														arr[3] = AnimVectorToVector(soutput.m_firstJointMS.GetTranslation());
														arr[4] = AnimVectorToVector(soutput.m_secondJointMS.GetTranslation());
														arr[5] = AnimVectorToVector(soutput.m_endBoneMS.GetTranslation());
													}
													#endif
												}

												{
													const Int32 boneIndexC = handR;
													const Int32 boneIndexB = boneIndexC != -1 ? sk->GetParentBoneIndex( boneIndexC ) : -1;
													const Int32 boneIndexA = boneIndexB != -1 ? sk->GetParentBoneIndex( boneIndexB ) : -1;
													const Int32 boneIndexParentA = boneIndexA != -1 ? sk->GetParentBoneIndex( boneIndexA ) : -1;

													AnimQsTransform boneMS_ParentA = pose.GetBoneMS(boneIndexParentA);
													AnimQsTransform boneLS_A = output.m_outputPose[boneIndexA];
													AnimQsTransform boneLS_B = output.m_outputPose[boneIndexB];
													AnimQsTransform boneLS_C = output.m_outputPose[boneIndexC];

													#ifndef NO_EDITOR
													{
														AnimQsTransform boneMS_A; boneMS_A.SetMul(boneMS_ParentA, boneLS_A);
														AnimQsTransform boneMS_B; boneMS_B.SetMul(boneMS_A, boneLS_B);
														AnimQsTransform boneMS_C; boneMS_C.SetMul(boneMS_B, boneLS_C);

														arr[6] = AnimVectorToVector(boneMS_A.GetTranslation());
														arr[7] = AnimVectorToVector(boneMS_B.GetTranslation());
														arr[8] = AnimVectorToVector(boneMS_C.GetTranslation());
													}
													#endif

													Ik2Solver::Input sinput;
													sinput.m_firstJointParentTransformMS = boneMS_ParentA;
													sinput.m_firstJointLS = boneLS_A;
													sinput.m_secondJointLS = boneLS_B;
													sinput.m_endBoneLS = boneLS_C;
													sinput.m_hingeAxisLS = VectorToAnimVector(BehaviorUtils::VectorFromAxis(A_Y));
													sinput.m_endTargetMS = boneMS_HandR.GetTranslation();

													Ik2Solver::Output soutput;

													Ik2Solver::Solve(sinput, soutput);

													output.m_outputPose[boneIndexA] = soutput.m_firstJointLS;
													output.m_outputPose[boneIndexB] = soutput.m_secondJointLS;
													output.m_outputPose[boneIndexC] = soutput.m_endBoneLS;

													#ifndef NO_EDITOR
													{
														arr[9] = AnimVectorToVector(soutput.m_firstJointMS.GetTranslation());
														arr[10] = AnimVectorToVector(soutput.m_secondJointMS.GetTranslation());
														arr[11] = AnimVectorToVector(soutput.m_endBoneMS.GetTranslation());
													}
													#endif
												}
											}
										}

										ImportAnimationUtils::ConvertToAdditiveFrame( inputPose->m_outputPose, output.m_outputPose, inputPose->m_numBones, m_additiveType );

										{
											const Vector& vPointA = instance[ i_targetA ];
											const Vector vecA = vPointA - AnimVectorToVector(boneB.GetTranslation());
											const Vector vecB = vPointB - AnimVectorToVector(boneB.GetTranslation());
											const Float angle = MathUtils::VectorUtils::GetAngleDegBetweenVectors( vecA, vecB );

											const Float headDownAngle = Clamp<Float>(m_headDownCurve->GetFloatValue(angle), 0.f, 90.f );
											const Float headDownMul = Clamp<Float>(m_headProgressCurve->GetFloatValue(progress), 0.f, 1.f );

											const RedVector4 axes = RedVector4( 0.f, 0.f, 1.f, 1.f );
											const RedQuaternion quat( axes, DEG2RAD( headDownMul * headDownAngle ) );

											RedQsTransform transform( RedVector4(0.0f, 0.0f, 0.0f, 0.0f ), quat, RedVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

											output.m_outputPose[ boneIdx+1 ].SetMul( output.m_outputPose[ boneIdx ], transform );
										}
									}
								}
							}
						}
						/*
						if ( useOnlyBaseCloud && !isTransitionToZero )
						{
							ASSERT( isTargetSetB );
							SampleInternal( context, instance, *poseB, boneMS, l2w, sampleWeight, vPointB, 0.f );

							const TDynArray< Int32 >& transitionBonesPred = instance[ i_transitionBonesPred ];
							const Uint32 numTransBones = transitionBonesPred.Size();

							for ( Uint32 i=0; i<numTransBones; ++i )
							{
								const Int32 bone = transitionBonesPred[ i ];
								ASSERT( bone != -1 );

								poseC->m_outputPose[ bone ].Lerp( poseC->m_outputPose[ bone ], poseB->m_outputPose[ bone ], progressPred );
							}
						}

						output.SetInterpolate( output, *poseC, progressTarget );
						*/
					}
				}
				else
				{
					ASSERT( 0 );
					//const Vector vPoint = CalcTarget( instance, boneMS );
					//SampleInternal( context, instance, output, boneMS, l2w, sampleWeight, vPoint, blendWeight );
				}
			}
		}
		else
		{
			ASSERT( 0 );
			//const Vector vPoint = CalcTarget( instance, boneMS );
			//ASSERT( isTargetSetB );
			//const Vector& vPoint = instance[ i_targetB ];
			//SampleInternal( context, instance, output, boneMS, l2w, weightValue, vPoint, blendWeight );
		}
	}
	else if ( !m_cachedInputNode && m_writeToPoseLikeAdditiveNode )
	{
		BehaviorUtils::BlendingUtils::SetPoseIdentity( output );
	}
}

void CBehaviorGraphPointCloudLookAtNode::SampleInternal( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const AnimQsTransform& boneMS, const AnimQsTransform& l2w, Float weight, const Vector& vPoint, Float blendWeight ) const
{
	if ( blendWeight < 0.01f )
	{
		SamplePointCloud( context, instance, output, boneMS, l2w, weight, vPoint, Pose_A );
	}
	else if ( blendWeight > 0.99f )
	{
		SamplePointCloud( context, instance, output, boneMS, l2w, weight, vPoint, Pose_B );
	}
	else
	{
		if ( m_additiveMode )
		{
			CCacheBehaviorGraphOutput cachePoseA( context );
			CCacheBehaviorGraphOutput cachePoseB( context );

			SBehaviorGraphOutput* poseA = cachePoseA.GetPose();
			SBehaviorGraphOutput* poseB = cachePoseB.GetPose();
			if ( poseA && poseB )
			{
				BehaviorUtils::BlendingUtils::SetPoseIdentity( *poseA );
				BehaviorUtils::BlendingUtils::SetPoseIdentity( *poseB );

				SamplePointCloud( context, instance, *poseA, boneMS, l2w, 1.f, vPoint, Pose_A );
				SamplePointCloud( context, instance, *poseB, boneMS, l2w, 1.f, vPoint, Pose_B );

				poseA->SetInterpolate( *poseA, *poseB, blendWeight );

				if ( !m_writeToPoseLikeAdditiveNode )
				{
					BehaviorUtils::BlendingUtils::BlendAdditive( output, output, *poseA, weight, m_additiveType );
				}
				else
				{
					BehaviorUtils::BlendingUtils::SetPoseIdentity( output );
					output.SetInterpolate( output, *poseA, weight );
				}
			}
		}
		else
		{
			CCacheBehaviorGraphOutput cachePoseA( context );
			CCacheBehaviorGraphOutput cachePoseB( context );

			SBehaviorGraphOutput* poseA = cachePoseA.GetPose();
			SBehaviorGraphOutput* poseB = cachePoseB.GetPose();
			if ( poseA && poseB )
			{
				SamplePointCloud( context, instance, *poseA, boneMS, l2w, 1.f, vPoint, Pose_A );
				SamplePointCloud( context, instance, *poseB, boneMS, l2w, 1.f, vPoint, Pose_B );

				poseA->SetInterpolate( *poseA, *poseB, blendWeight );

				output.SetInterpolate( output, *poseA, weight );
			}
		}
	}
}

void CBehaviorGraphPointCloudLookAtNode::SamplePointCloud( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const AnimQsTransform& boneMS, const AnimQsTransform& l2w, Float weight, const Vector& vPoint, EPoseNum poseNum ) const
{
	const CAnimPointCloudLookAtParam* params = poseNum == Pose_A ? instance[ i_paramsA ] : instance[ i_paramsB ];
	AnimPointCloud::SphereLookAt::TQueryCookie& cookie = poseNum == Pose_A ? instance[ i_cookieA ] : instance[ i_cookieB ];	

	if ( params && weight > 0.f )
	{
		AnimPointCloud::SphereLookAt::Input lookAtInput;
		lookAtInput.m_boneMS = boneMS;
		lookAtInput.m_localToWorld = l2w;
		lookAtInput.m_params = params;
		lookAtInput.m_pointWS = RedVector4( vPoint.X, vPoint.Y, vPoint.Z );

		AnimPointCloud::SphereLookAt::Output lookAtOutput;

		if ( AnimPointCloud::SphereLookAt::DoLookAt( lookAtInput, lookAtOutput, cookie ) )
		{
			if ( m_additiveMode )
			{
				// Skip first pose because first pose is reference pose for additive mode
				for ( Uint32 i=0; i<3; ++i )
				{
					if ( lookAtOutput.m_poses[ i ] != -1 )
					{
						lookAtOutput.m_poses[ i ] += 1;
					}
				}
			}

			if ( lookAtOutput.m_poses[ 0 ] != -1 && lookAtOutput.m_poses[ 1 ] != -1 && lookAtOutput.m_poses[ 2 ] != -1 )
			{
				BlendPoses3( context, instance, output, lookAtOutput.m_poses, lookAtOutput.m_weights, weight, poseNum );
			}
			else if ( lookAtOutput.m_poses[ 0 ] != -1 && lookAtOutput.m_poses[ 1 ] != -1 )
			{
				BlendPoses2( context, instance, output, lookAtOutput.m_poses[ 0 ], lookAtOutput.m_poses[ 1 ], lookAtOutput.m_weights[ 0 ], lookAtOutput.m_weights[ 1 ], weight, poseNum );
			}
			else if ( lookAtOutput.m_poses[ 1 ] != -1 && lookAtOutput.m_poses[ 2 ] != -1 )
			{
				BlendPoses2( context, instance, output, lookAtOutput.m_poses[ 1 ], lookAtOutput.m_poses[ 2 ], lookAtOutput.m_weights[ 1 ], lookAtOutput.m_weights[ 2 ], weight, poseNum );
			}
			else if ( lookAtOutput.m_poses[ 0 ] != -1 && lookAtOutput.m_poses[ 2 ] != -1 )
			{
				BlendPoses2( context, instance, output, lookAtOutput.m_poses[ 0 ], lookAtOutput.m_poses[ 2 ], lookAtOutput.m_weights[ 0 ], lookAtOutput.m_weights[ 2 ], weight, poseNum );
			}
			else
			{
				RED_ASSERT( 0, TXT("Bug - CBehaviorGraphPointCloudLookAtNode") );
			}
		}
		else
		{
			RED_ASSERT( 0, TXT("Bug - CBehaviorGraphPointCloudLookAtNode") );
		}

#ifndef NO_EDITOR
		{
			AnimQsTransform boneWS_Pre;
			boneWS_Pre.SetMul( l2w, boneMS );

			const Int32 boneIndex = instance[ i_boneIndex ];
			const AnimQsTransform boneMS_Post = output.GetBoneModelTransform( instance.GetAnimatedComponent(), boneIndex );
			AnimQsTransform boneWS_Post;
			boneWS_Post.SetMul( l2w, boneMS_Post );

			RedVector4 boneDirLS = VectorToAnimVector( params->GetDirectionLS() );
			RedVector4 boneDirWS;
			boneDirWS.RotateDirection( boneWS_Pre.Rotation, boneDirLS );

			instance[ i_lastBoneWS_Pre ][ poseNum ] = AnimQsTransformToMatrix( boneWS_Pre );
			instance[ i_lastBoneWS_Post ][ poseNum ] = AnimQsTransformToMatrix( boneWS_Post );
			instance[ i_lastBoneDir ][ poseNum ] = AnimVectorToVector( boneDirWS );
			instance[ i_lastPointPosOnSphere ][ poseNum ] = lookAtOutput.m_pointBS;

			instance[ i_lastNNpoint ][ poseNum ] = lookAtOutput.m_nearestPoint;

			TDynArray< Int32 >& lastTri = instance[ i_lastTri ][ poseNum ];
			TDynArray< Float >& lastWeight = instance[ i_lastWeights ][ poseNum ];

			for ( Uint32 i=0; i<3; ++i )
			{
				lastTri[ i ] = lookAtOutput.m_poses[ i ];
				lastWeight[ i ] = lookAtOutput.m_weights[ i ];
			}
		}
#endif
	}
}

void CBehaviorGraphPointCloudLookAtNode::BlendPoses3( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Int32 poses[3], Float weights[3], Float mainWeight, EPoseNum poseNum ) const
{
	ASSERT( poses[ 0 ] != -1 );
	ASSERT( poses[ 1 ] != -1 );
	ASSERT( poses[ 2 ] != -1 );
	
	ASSERT( weights[0] >= 0.f && weights[0] <= 1.f );
	ASSERT( weights[1] >= 0.f && weights[1] <= 1.f );
	ASSERT( weights[2] >= 0.f && weights[2] <= 1.f );

	ASSERT( mainWeight >= 0.f && mainWeight <= 1.f );

	CCacheBehaviorGraphOutput cachePose1( context );
	CCacheBehaviorGraphOutput cachePose2( context );
	CCacheBehaviorGraphOutput cachePose3( context );

	SBehaviorGraphOutput* temp1 = cachePose1.GetPose();
	SBehaviorGraphOutput* temp2 = cachePose2.GetPose();
	SBehaviorGraphOutput* temp3 = cachePose3.GetPose();

	if ( temp1 && temp2 && temp3 )
	{
		SampleAnimation( instance, *temp1, poses[ 0 ], poseNum );
		SampleAnimation( instance, *temp2, poses[ 1 ], poseNum );
		SampleAnimation( instance, *temp3, poses[ 2 ], poseNum );

		ASSERT( MAbs( weights[0] + weights[1] + weights[2] - 1.f ) < 0.01f );

		if ( mainWeight < 1.f || m_additiveMode )
		{
			CCacheBehaviorGraphOutput cachePose( context );
			SBehaviorGraphOutput* temp = cachePose.GetPose();
			if ( temp )
			{
				BehaviorUtils::BlendingUtils::SetPoseZero( *temp );

				BehaviorUtils::BlendingUtils::BlendPosesNormal( *temp, *temp1, weights[0] );
				BehaviorUtils::BlendingUtils::BlendPosesNormal( *temp, *temp2, weights[1] );
				BehaviorUtils::BlendingUtils::BlendPosesNormal( *temp, *temp3, weights[2] );

				BehaviorUtils::BlendingUtils::RenormalizePose( *temp, 1.f );

				if ( m_additiveMode )
				{
					if ( ConvertAnimationToAdditiveFlag( poseNum ) )
					{
						ConvertPoseToAdditive( instance, *temp, poseNum );
					}

					if ( !m_writeToPoseLikeAdditiveNode )
					{
						BehaviorUtils::BlendingUtils::BlendAdditive( output, output, *temp, mainWeight, m_additiveType );
					}
					else
					{
						BehaviorUtils::BlendingUtils::SetPoseIdentity( output );
						output.SetInterpolate( output, *temp, mainWeight );

						BlendOutLowerBodyParts( instance, output );
					}
				}
				else
				{
					output.SetInterpolate( output, *temp, mainWeight );
				}
			}
		}
		else
		{
			BehaviorUtils::BlendingUtils::SetPoseZero( output );

			BehaviorUtils::BlendingUtils::BlendPosesNormal( output, *temp1, weights[0] );
			BehaviorUtils::BlendingUtils::BlendPosesNormal( output, *temp2, weights[1] );
			BehaviorUtils::BlendingUtils::BlendPosesNormal( output, *temp3, weights[2] );

			BehaviorUtils::BlendingUtils::RenormalizePose( output, 1.f );
		}
	}
}

void CBehaviorGraphPointCloudLookAtNode::BlendPoses2( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Int32 poseA, Int32 poseB, Float weightA, Float weightB, Float mainWeight, EPoseNum poseNum ) const
{
	ASSERT( poseA != -1 );
	ASSERT( poseB != -1 );

	ASSERT( weightA >= 0.f && weightA <= 1.f );
	ASSERT( weightB >= 0.f && weightB <= 1.f );

	ASSERT( mainWeight >= 0.f && mainWeight <= 1.f );

	CCacheBehaviorGraphOutput cachePose1( context );
	CCacheBehaviorGraphOutput cachePose2( context );

	SBehaviorGraphOutput* temp1 = cachePose1.GetPose();
	SBehaviorGraphOutput* temp2 = cachePose2.GetPose();

	if ( temp1 && temp2 )
	{
		SampleAnimation( instance, *temp1, poseA, poseNum );
		SampleAnimation( instance, *temp2, poseB, poseNum );

		ASSERT( MAbs( weightA + weightB - 1.f ) < 0.01f );

		if ( mainWeight < 1.f || m_additiveMode )
		{
			temp1->SetInterpolate( *temp1, *temp2, weightB );

			if ( m_additiveMode )
			{
				if ( ConvertAnimationToAdditiveFlag( poseNum ) )
				{
					ConvertPoseToAdditive( instance, *temp1, poseNum );
				}

				if ( !m_writeToPoseLikeAdditiveNode )
				{
					BehaviorUtils::BlendingUtils::BlendAdditive( output, output, *temp1, mainWeight, m_additiveType );
				}
				else
				{
					BehaviorUtils::BlendingUtils::SetPoseIdentity( output );
					output.SetInterpolate( output, *temp1, mainWeight );

					BlendOutLowerBodyParts( instance, output );
				}
			}
			else
			{
				output.SetInterpolate( output, *temp1, mainWeight );
			}
		}
		else
		{
			output.SetInterpolate( *temp1, *temp2, weightB );
		}
	}
}

void CBehaviorGraphPointCloudLookAtNode::BlendOutLowerBodyParts( CBehaviorGraphInstance& instance, SBehaviorGraphOutput& output ) const
{
	const Float lowerBodyPartsWeight = instance[ i_lowerBodyPartsWeight ];
	ASSERT( lowerBodyPartsWeight >= 0.f && lowerBodyPartsWeight <= 1.f );
	if ( lowerBodyPartsWeight < 1.f )
	{
		const TDynArray< Int32 >& lowerBodyPartBones = instance[ i_lowerBodyPartBones ];
		const Uint32 numBones = lowerBodyPartBones.Size();
		for ( Uint32 i=0; i<numBones; ++i )
		{
			const Int32 boneIdx = lowerBodyPartBones[ i ];

			ASSERT( boneIdx != -1 );
			ASSERT( boneIdx < (Int32)output.m_numBones );

			if ( boneIdx != -1 && boneIdx < (Int32)output.m_numBones )
			{
				output.m_outputPose[ boneIdx ].SetIdentity();
			}
		}
	}
}

Bool CBehaviorGraphPointCloudLookAtNode::ConvertAnimationToAdditiveFlag( EPoseNum poseNum ) const
{
	return poseNum == Pose_A ? m_convertAnimationToAdditiveFlagA : m_convertAnimationToAdditiveFlagB;
}

/*Vector CBehaviorGraphPointCloudLookAtNode::CalcTarget( CBehaviorGraphInstance& instance, const AnimQsTransform& boneMS ) const
{
	const Float progress = instance[ i_progress ];
	const Float targetWeight = MapTargetWeight( instance, progress );

	if ( targetWeightUnmod > 0.f )
	{
		ASSERT( instance[ i_targetSetB ] );
	}
	if ( targetWeightUnmod < 1.f )
	{
		ASSERT( instance[ i_targetSetA ] );
	}

	IBehaviorGraphPointCloudLookAtTransition* transition = instance[ i_transition ];
	if ( transition )
	{
		IBehaviorGraphPointCloudLookAtTransition::Input input;

		input.m_targetA = instance[ i_targetA ];
		input.m_targetB = instance[ i_targetB ];
		input.m_targetBlend = targetWeight;
		input.m_weight = instance[ i_progress ];
		input.m_transitionWeight = instance[ i_transitionWeight ];
		input.m_boneMS = boneMS;

		// TODO - we always take poseA data
		const CAnimPointCloudLookAtParam* params = instance[ i_paramsA ];
		input.m_boneDirLS = params ? params->GetDirectionLS() : Vector::EX;

		return transition->CalcTarget( instance, input );
	}
	else
	{
		const Vector& vPointA = instance[ i_targetA ];
		const Vector& vPointB = instance[ i_targetB ];

		return Vector::Interpolate( vPointA, vPointB, targetWeight );
	}
}*/

Float CBehaviorGraphPointCloudLookAtNode::MapTargetWeight( CBehaviorGraphInstance& instance, Float weight ) const
{
	const Float finalValue = Clamp( m_targetWeightCurve->GetFloatValue( weight ), 0.f, 1.f );
	return finalValue;
}

/*Float CBehaviorGraphPointCloudLookAtNode::GetTargetWeight( CBehaviorGraphInstance& instance ) const
{
	const Float targetWeight = instance[ i_targetWeight ];
	const Float finalValue = Clamp( m_targetWeightCurve->GetFloatValue( targetWeight ), 0.f, 1.f );
	return finalValue;
}*/

/*Vector CBehaviorGraphPointCloudLookAtNode::GetTarget( CBehaviorGraphInstance& instance ) const
{
	IBehaviorGraphPointCloudLookAtTransition* transition = instance[ i_transition ];
	if ( transition )
	{
		return transition->GetTarget( instance );
	}
	else
	{
		const Vector& vPointA = instance[ i_targetA ];
		const Vector& vPointB = instance[ i_targetB ];
		const Float targetWeight = GetTargetWeight( instance );

		return Vector::Interpolate( vPointA, vPointB, targetWeight );
	}
}*/

void CBehaviorGraphPointCloudLookAtNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
#ifndef NO_EDITOR
	static Bool FORCE_DRAW = false;
	if ( m_generateEditorFragments || FORCE_DRAW )
#else
	if ( m_generateEditorFragments )
#endif
	{
#ifndef NO_EDITOR
		{
			const TDynArray< Vector >& arr = instance[ i_handPoints ];

			Color colorA(0,255,0);
			Color colorB(255,0,0);

			frame->AddDebugLine(arr[0],arr[1], colorA, true, true);
			frame->AddDebugLine(arr[1],arr[2], colorA, true, true);

			frame->AddDebugLine(arr[3],arr[4], colorB, true, true);
			frame->AddDebugLine(arr[4],arr[5], colorB, true, true);

			frame->AddDebugLine(arr[6],arr[7], colorA, true, true);
			frame->AddDebugLine(arr[7],arr[8], colorA, true, true);

			frame->AddDebugLine(arr[9],arr[10], colorB, true, true);
			frame->AddDebugLine(arr[10],arr[11], colorB, true, true);
		}
#endif

		if ( instance[ i_progress ] > 0.f )
		{
			const Vector& vPointA = instance[ i_targetA ];
			const Vector& vPointB = instance[ i_targetB ];

			const Bool isTargetSetA = instance[ i_targetSetA ];
			const Bool isTargetSetB = instance[ i_targetSetB ];

			if ( isTargetSetA )
			{
				frame->AddDebugSphere( vPointA, 0.1f, Matrix::IDENTITY, Color( 128, 128, 0 ) );
				frame->AddDebugSphere( vPointA, 0.01f, Matrix::IDENTITY, Color( 128, 128, 0 ) );
			}

			if ( isTargetSetB )
			{
				frame->AddDebugSphere( vPointB, 0.1f, Matrix::IDENTITY, Color( 255, 255, 0 ) );
				frame->AddDebugSphere( vPointB, 0.01f, Matrix::IDENTITY, Color( 255, 255, 0 ) );

				DrawPointCloundData( instance, frame, vPointB, Pose_A );
			}

			/*const Float blend = instance[ i_blendWeight ];
			if ( blend > 0.99f )
			{
				DrawPointCloundData( instance, frame, vPointA, Pose_B );
			}
			else if ( blend < 0.01f )
			{
				DrawPointCloundData( instance, frame, vPointA, Pose_A );
			}
			else
			{
				DrawPointCloundData( instance, frame, vPointA, Pose_A );
				DrawPointCloundData( instance, frame, vPointA, Pose_B );
			}*/
		}
	}
}

void CBehaviorGraphPointCloudLookAtNode::DrawPointCloundData( CBehaviorGraphInstance& instance, CRenderFrame* frame, const Vector& vPoint, EPoseNum poseNum ) const
{
#ifndef NO_EDITOR

	if ( instance[ i_progress ] <= 0.f )
	{
		return;
	}

	const Matrix& boneWS_Pre = instance[ i_lastBoneWS_Pre ][ poseNum ];
	//const Matrix& boneWS_Post = instance[ i_lastBoneWS_Post ][ poseNum ];

	const CAnimPointCloudLookAtParam* params = poseNum == Pose_A ? instance[ i_paramsA ] : instance[ i_paramsB ];
	if ( params )
	{
		AnimPointCloud::Render::DrawPointCloud( frame, boneWS_Pre, params );
	}
	else
	{
		return;
	}

	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	const TDynArray< AnimPointCloud::TPoint >& pointsBS = params->GetPointsBS();

	const Int32 boneIndex = instance[ i_boneIndex ];
	if ( boneIndex != -1 )
	{
		{
			const Vector& bDir = instance[ i_lastBoneDir ][ poseNum ];

			frame->AddDebugLine( boneWS_Pre.GetTranslation(), boneWS_Pre.GetTranslation() + bDir, Color( 255, 0, 0 ) );

			frame->AddDebugLine( boneWS_Pre.GetTranslation(), vPoint, Color( 255, 255, 0 ) );
		}

		{
			const Matrix boneWS_Curr = ac->GetBoneMatrixWorldSpace( boneIndex );
			const Vector bDir = boneWS_Curr.TransformVector( params->GetDirectionLS() );

			frame->AddDebugLine( boneWS_Curr.GetTranslation(), boneWS_Curr.GetTranslation() + bDir, Color( 255, 0, 0 ) );
			frame->AddDebugLine( boneWS_Curr.GetTranslation(), vPoint, Color( 255, 255, 0 ) );
		}
	}

	Matrix bsToWsMatrix;
	params->CalcMatrixBSToWS( boneWS_Pre, bsToWsMatrix );

	{
		const Vector pWS = bsToWsMatrix.TransformPoint( instance[ i_lastPointPosOnSphere ][ poseNum ] );

		frame->AddDebugSphere( pWS, 0.01f, Matrix::IDENTITY, Color( 255, 0, 0 ) );
	}

	const Int32 nnIndex = instance[ i_lastNNpoint ][ poseNum ];
	if ( nnIndex != -1 && nnIndex < pointsBS.SizeInt() )
	{
		const Vector pWS = bsToWsMatrix.TransformPoint( pointsBS[ nnIndex ] );

		frame->AddDebugSphere( pWS, 0.015f, Matrix::IDENTITY, Color( 128, 255, 128 ) );
	}

	const TDynArray< Int32 >& lastTri = instance[ i_lastTri ][ poseNum ];
	const TDynArray< Float >& lastWeight = instance[ i_lastWeights ][ poseNum ];

	static Bool SHOW_TRI_DESC = true;

	for ( Int32 i=0; i<3; ++i )
	{
		if ( lastTri[ i ] != -1 && lastTri[ i ] < pointsBS.SizeInt() )
		{
			const Vector pWS = bsToWsMatrix.TransformPoint( pointsBS[ lastTri[ i ] ] );

			frame->AddDebugSphere( pWS, 0.025f, Matrix::IDENTITY, Color( 255, 255, 0 ) );

			if ( SHOW_TRI_DESC )
			{
				String text = String::Printf( TXT("(%d)[%d]%1.2f|"), i, lastTri[ i ], lastWeight[ i ] );
				frame->AddDebugText( pWS, text, true, Color( 255, 255, 255, 200 ), Color( 0, 0, 0, 200 ) );
			}
		}
	}
#endif
}

void CBehaviorGraphPointCloudLookAtNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedTargetNodeA )
	{
		m_cachedTargetNodeA->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedTargetNodeB )
	{
		m_cachedTargetNodeB->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedFallbackNode )
	{
		m_cachedFallbackNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedProgressNode )
	{
		m_cachedProgressNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedDurationNode )
	{
		m_cachedDurationNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedWeightANode )
	{
		m_cachedWeightANode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedWeightBNode )
	{
		m_cachedWeightBNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedUseSecBlendANode )
	{
		m_cachedUseSecBlendANode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedUseSecBlendBNode )
	{
		m_cachedUseSecBlendBNode->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphPointCloudLookAtNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedTargetNodeA )
	{
		m_cachedTargetNodeA->Activate( instance );
	}

	if ( m_cachedTargetNodeB )
	{
		m_cachedTargetNodeB->Activate( instance );
	}

	if ( m_cachedFallbackNode )
	{
		m_cachedFallbackNode->Activate( instance );
	}

	if ( m_cachedProgressNode )
	{
		m_cachedProgressNode->Activate( instance );
	}

	if ( m_cachedWeightANode )
	{
		m_cachedWeightANode->Activate( instance );
	}

	if ( m_cachedWeightBNode )
	{
		m_cachedWeightBNode->Activate( instance );
	}

	if ( m_cachedUseSecBlendANode )
	{
		m_cachedUseSecBlendANode->Activate( instance );
	}

	if ( m_cachedUseSecBlendBNode )
	{
		m_cachedUseSecBlendBNode->Activate( instance );
	}

	if ( m_cachedDurationNode )
	{
		m_cachedDurationNode->Activate( instance );
	}

	if ( m_additiveMode && m_convertAnimationToAdditiveFlagA )
	{
		TryCachePoseForAdditiveMode( instance, Pose_A );
	}

	if ( m_additiveMode && m_convertAnimationToAdditiveFlagB )
	{
		TryCachePoseForAdditiveMode( instance, Pose_B );
	}
}

void CBehaviorGraphPointCloudLookAtNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	FreePose( instance, Pose_A );
	FreePose( instance, Pose_B );

	TBaseClass::OnDeactivated( instance );

	if ( m_cachedTargetNodeA )
	{
		m_cachedTargetNodeA->Deactivate( instance );
	}

	if ( m_cachedTargetNodeB )
	{
		m_cachedTargetNodeB->Deactivate( instance );
	}

	if ( m_cachedFallbackNode )
	{
		m_cachedFallbackNode->Deactivate( instance );
	}

	if ( m_cachedProgressNode )
	{
		m_cachedProgressNode->Deactivate( instance );
	}

	if ( m_cachedWeightANode )
	{
		m_cachedWeightANode->Deactivate( instance );
	}

	if ( m_cachedWeightBNode )
	{
		m_cachedWeightBNode->Deactivate( instance );
	}

	if ( m_cachedUseSecBlendANode )
	{
		m_cachedUseSecBlendANode->Deactivate( instance );
	}

	if ( m_cachedUseSecBlendBNode )
	{
		m_cachedUseSecBlendBNode->Deactivate( instance );
	}

	if ( m_cachedDurationNode )
	{
		m_cachedDurationNode->Deactivate( instance );
	}
}

void CBehaviorGraphPointCloudLookAtNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedProgressNode = CacheValueBlock( TXT("Progress") );
	m_cachedWeightANode = CacheValueBlock( TXT("WeightA") );
	m_cachedWeightBNode = CacheValueBlock( TXT("WeightB") );
	m_cachedTargetNodeA = CacheVectorValueBlock( TXT("TargetA") );
	m_cachedTargetNodeB = CacheVectorValueBlock( TXT("TargetB") );
	m_cachedUseSecBlendANode = CacheValueBlock( TXT("UseSecA") );
	m_cachedUseSecBlendBNode = CacheValueBlock( TXT("UseSecB") );
	m_cachedFallbackNode = CacheBlock( TXT("Fallback") );
	m_cachedDurationNode = CacheValueBlock( TXT("Duration") );
}

//////////////////////////////////////////////////////////////////////////

void CBehaviorGraphPointCloudLookAtNode::SetTargetA( CBehaviorGraphInstance& instance, const Vector& point ) const
{
	instance[ i_targetA ] = point;
	instance[ i_targetSetA ] = true;
}

void CBehaviorGraphPointCloudLookAtNode::SetTargetB( CBehaviorGraphInstance& instance, const Vector& point ) const
{
	instance[ i_targetB ] = point;
	instance[ i_targetSetB ] = true;
}

void CBehaviorGraphPointCloudLookAtNode::ResetTargetA( CBehaviorGraphInstance& instance ) const
{
	instance[ i_targetA ] = Vector::ZERO_3D_POINT;
	instance[ i_targetSetA ] = false;
}

void CBehaviorGraphPointCloudLookAtNode::ResetTargetB( CBehaviorGraphInstance& instance ) const
{
	instance[ i_targetB ] = Vector::ZERO_3D_POINT;
	instance[ i_targetSetB ] = false;
}

/*void CBehaviorGraphPointCloudLookAtNode::SetWeight( CBehaviorGraphInstance& instance, Float progress, Float weightValue ) const
{
	instance[ i_progress ] = Clamp( progress, 0.f, 1.f );
	instance[ i_weightValue ] = Clamp( weightValue, 0.f, 1.f );
}

void CBehaviorGraphPointCloudLookAtNode::SetBlendWeight( CBehaviorGraphInstance& instance, Float w ) const
{
	instance[ i_blendWeight ] = Clamp( w, 0.f, 1.f );
}

void CBehaviorGraphPointCloudLookAtNode::SetTargetWeight( CBehaviorGraphInstance& instance, Float w ) const
{
	instance[ i_targetWeight ] = Clamp( w, 0.f, 1.f );
}

void CBehaviorGraphPointCloudLookAtNode::SetTransitionWeight( CBehaviorGraphInstance& instance, Float w ) const
{
	instance[ i_transitionWeight ] = w;
}
*/
void CBehaviorGraphPointCloudLookAtNode::SetLowerBodyPartsWeight( CBehaviorGraphInstance& instance, Float w ) const
{
	instance[ i_lowerBodyPartsWeight ] = w;
}

void CBehaviorGraphPointCloudLookAtNode::ResetParams( CBehaviorGraphInstance& instance ) const
{
	instance[ i_progress ] = 0.f;
	instance[ i_weightA ] = 0.f;
	instance[ i_weightB ] = 0.f;
	instance[ i_useSecBlendCloudA ] = false;
	instance[ i_useSecBlendCloudB ] = false;
	instance[ i_duration ] = 0.f;
	instance[ i_useDeformationMS ] = true;
}

void CBehaviorGraphPointCloudLookAtNode::SetParams( CBehaviorGraphInstance& instance, const CBehaviorGraphPointCloudLookAtNode::Params& params ) const
{
	instance[ i_progress ] = params.m_progress;
	instance[ i_weightA ] = params.m_weightA;
	instance[ i_weightB ] = params.m_weightB;
	instance[ i_useSecBlendCloudA ] = params.m_useSecBlendCloudA;
	instance[ i_useSecBlendCloudB ] = params.m_useSecBlendCloudB;
	instance[ i_duration ] = params.m_duration;
	instance[ i_useDeformationMS ] = params.m_useDeformationMS;
}

//////////////////////////////////////////////////////////////////////////

CBehaviorPointCloudLookAtInterface::CBehaviorPointCloudLookAtInterface()
	: m_node( nullptr )
	, m_instance( nullptr )
{

}

CBehaviorPointCloudLookAtInterface::~CBehaviorPointCloudLookAtInterface()
{

}

void CBehaviorPointCloudLookAtInterface::Init( CBehaviorGraphPointCloudLookAtNode* node, CBehaviorGraphInstance* instance )
{
	m_node = node;
	m_instance = instance;
}

void CBehaviorPointCloudLookAtInterface::Clear()
{
	m_node = nullptr;
	m_instance = nullptr;
}

Bool CBehaviorPointCloudLookAtInterface::IsValid() const
{
	CBehaviorGraphPointCloudLookAtNode* node = nullptr;
	CBehaviorGraphInstance* instance = nullptr;
	return IsValid( node, instance );
}

Bool CBehaviorPointCloudLookAtInterface::IsValid( CBehaviorGraphPointCloudLookAtNode*& node, CBehaviorGraphInstance*& instance ) const
{
	node = m_node.Get();
	instance = m_instance.Get();
	return node && instance && instance->IsBinded();
}

void CBehaviorPointCloudLookAtInterface::SetAnimations( const CName& animationA, const CName& animationB )
{
	CBehaviorGraphPointCloudLookAtNode* node = nullptr;
	CBehaviorGraphInstance* instance = nullptr;

	if ( IsValid( node, instance ) )
	{
		node->SetAnimations( *instance, animationA, animationB );
	}
}

void CBehaviorPointCloudLookAtInterface::SetTargetA( const Vector& point )
{
	CBehaviorGraphPointCloudLookAtNode* node = nullptr;
	CBehaviorGraphInstance* instance = nullptr;

	if ( IsValid( node, instance ) )
	{
		node->SetTargetA( *instance, point );
	}
}

void CBehaviorPointCloudLookAtInterface::SetTargetB( const Vector& point )
{
	CBehaviorGraphPointCloudLookAtNode* node = nullptr;
	CBehaviorGraphInstance* instance = nullptr;

	if ( IsValid( node, instance ) )
	{
		node->SetTargetB( *instance, point );
	}
}

void CBehaviorPointCloudLookAtInterface::ResetTargetA()
{
	CBehaviorGraphPointCloudLookAtNode* node = nullptr;
	CBehaviorGraphInstance* instance = nullptr;

	if ( IsValid( node, instance ) )
	{
		node->ResetTargetA( *instance );
	}
}

void CBehaviorPointCloudLookAtInterface::ResetTargetB()
{
	CBehaviorGraphPointCloudLookAtNode* node = nullptr;
	CBehaviorGraphInstance* instance = nullptr;

	if ( IsValid( node, instance ) )
	{
		node->ResetTargetB( *instance );
	}
}

/*void CBehaviorPointCloudLookAtInterface::SetWeight( Float progress, Float weightValue )
{
	CBehaviorGraphPointCloudLookAtNode* node = nullptr;
	CBehaviorGraphInstance* instance = nullptr;

	if ( IsValid( node, instance ) )
	{
		node->SetWeight( *instance, progress, weightValue );
	}
}

void CBehaviorPointCloudLookAtInterface::SetBlendWeight( Float w )
{
	CBehaviorGraphPointCloudLookAtNode* node = nullptr;
	CBehaviorGraphInstance* instance = nullptr;

	if ( IsValid( node, instance ) )
	{
		node->SetBlendWeight( *instance, w );
	}
}

void CBehaviorPointCloudLookAtInterface::SetTargetWeight( Float w )
{
	CBehaviorGraphPointCloudLookAtNode* node = nullptr;
	CBehaviorGraphInstance* instance = nullptr;

	if ( IsValid( node, instance ) )
	{
		node->SetTargetWeight( *instance, w );
	}
}

void CBehaviorPointCloudLookAtInterface::SetTransitionWeight( Float w )
{
	CBehaviorGraphPointCloudLookAtNode* node = nullptr;
	CBehaviorGraphInstance* instance = nullptr;

	if ( IsValid( node, instance ) )
	{
		node->SetTransitionWeight( *instance, w );
	}
}*/

void CBehaviorPointCloudLookAtInterface::SetLowerBodyPartsWeight( Float w )
{
	CBehaviorGraphPointCloudLookAtNode* node = nullptr;
	CBehaviorGraphInstance* instance = nullptr;

	if ( IsValid( node, instance ) )
	{
		node->SetLowerBodyPartsWeight( *instance, w );
	}
}

void CBehaviorPointCloudLookAtInterface::ResetParams()
{
	CBehaviorGraphPointCloudLookAtNode* node = nullptr;
	CBehaviorGraphInstance* instance = nullptr;

	if ( IsValid( node, instance ) )
	{
		node->ResetParams( *instance );
	}
}

void CBehaviorPointCloudLookAtInterface::SetParams( const CBehaviorGraphPointCloudLookAtNode::Params& params )
{
	CBehaviorGraphPointCloudLookAtNode* node = nullptr;
	CBehaviorGraphInstance* instance = nullptr;

	if ( IsValid( node, instance ) )
	{
		node->SetParams( *instance, params );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( IBehaviorGraphPointCloudLookAtTransition_Vector );

void IBehaviorGraphPointCloudLookAtTransition_Vector::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_target;
}

void IBehaviorGraphPointCloudLookAtTransition_Vector::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_target ] = Vector::ZERO_3D_POINT;
}

Vector IBehaviorGraphPointCloudLookAtTransition_Vector::CalcTarget( CBehaviorGraphInstance& instance, const IBehaviorGraphPointCloudLookAtTransition::Input& input ) const
{
	const Vector target = Vector::Interpolate( input.m_targetA, input.m_targetB, input.m_targetBlend );

	instance[ i_target ] = target;

	return target;
}

Vector IBehaviorGraphPointCloudLookAtTransition_Vector::GetTarget( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_target ];
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphPointCloudLookAtTransition_Vertical );

CBehaviorGraphPointCloudLookAtTransition_Vertical::CBehaviorGraphPointCloudLookAtTransition_Vertical()
	: m_maxAngleDiffDeg( 90.f )
	, m_scale( 5.f )
	, m_minAngle( -30.f )
	, m_maxAngle( 30.f )
{

}

Vector CBehaviorGraphPointCloudLookAtTransition_Vertical::CalcTarget( CBehaviorGraphInstance& instance, const IBehaviorGraphPointCloudLookAtTransition::Input& input ) const
{
	/*if ( m_curve )
	{
		const Vector bone = AnimVectorToVector( boneMS.Translation );

		const Float angleDegRatio = Min( MathUtils::VectorUtils::GetAngleDegBetweenVectors( targetA - bone, targetB - bone ), m_maxAngleDiffDeg ) / m_maxAngleDiffDeg;
		ASSERT( angleDegRatio >= 0.f );

		const Float zCorrection = m_curve->GetFloatValue( targetBlend );

		Vector target = Vector::Interpolate( targetA, targetB, targetBlend );

		{
			AnimVector4 targetSS = VectorToAnimVector( target );
			BehaviorUtils::SphericalFromCartesian( targetSS );

			targetSS.Z = RAD2DEG( targetSS.Z );
			targetSS.Z += transitionWeight * Clamp( m_scale * angleDegRatio * zCorrection, m_minAngle, m_maxAngle );	
			targetSS.Z = DEG2RAD( targetSS.Z );

			BehaviorUtils::CartesianFromSpherical( targetSS );
			target = AnimVectorToVector( targetSS );
		}

		instance[ i_target ] = target;

		return GetTarget( instance );
	}*/

	return TBaseClass::CalcTarget( instance, input );
}

#ifndef NO_EDITOR

void CBehaviorGraphPointCloudLookAtTransition_Vertical::OnCreatedInEditor()
{
	if ( !m_curve )
	{
		m_curve = CreateObject< CCurve >( this );
	}
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphPointCloudLookAtTransition_Spherical );

Vector CBehaviorGraphPointCloudLookAtTransition_Spherical::CalcTarget( CBehaviorGraphInstance& instance, const IBehaviorGraphPointCloudLookAtTransition::Input& input ) const
{
	const Matrix& l2w = instance.GetAnimatedComponent()->GetThisFrameTempLocalToWorld();

	const Vector bone = AnimVectorToVector( input.m_boneMS.Translation );
	const Vector boneWS = l2w.TransformPoint( bone );

	const Vector vecA = input.m_targetA - boneWS;
	const Vector vecB = input.m_targetB - boneWS;

	Vector boneDirLS = l2w.V[1].Normalized3();

	AnimVector4 targetASS = VectorToAnimVector( vecA );
	BehaviorUtils::SphericalFromCartesian( targetASS );
	AnimVector4 targetBSS = VectorToAnimVector( vecB );
	BehaviorUtils::SphericalFromCartesian( targetBSS );
	AnimVector4 boneSS = VectorToAnimVector( boneDirLS );
	BehaviorUtils::SphericalFromCartesian( boneSS );

	Float boneSS_a = RAD2DEG( boneSS.Y );

	Float targetASS_a = RAD2DEG( targetASS.Y );
	Float targetASS_b = RAD2DEG( targetASS.Z );
	Float targetBSS_a = RAD2DEG( targetBSS.Y );
	Float targetBSS_b = RAD2DEG( targetBSS.Z );

	Float angDistY = EulerAngles::AngleDistance( targetASS_a, targetBSS_a );
	Float angDistZ = EulerAngles::AngleDistance( targetASS_b, targetBSS_b );

	Float angleAY_MS  = EulerAngles::NormalizeAngle180( targetASS_a-boneSS_a );
	Float angleBY_MS  = EulerAngles::NormalizeAngle180( targetBSS_a-boneSS_a );

	if ( angleAY_MS + angDistY > 180.f )
	{
		angDistY += -360.f;
	}
	else if( angleAY_MS + angDistY < -180.f)
	{
		angDistY += 360.f;
	}

	const Float weight = input.m_targetBlend;

	const Float angY = targetASS_a + angDistY * weight;
	const Float angZ = targetASS_b + angDistZ * weight;
	
	AnimVector4 out;
	out.X = Lerp( weight, targetASS.X, targetBSS.X );
	out.Y = DEG2RAD( angY );
	out.Z = DEG2RAD( angZ );

	BehaviorUtils::CartesianFromSpherical( out );
	Vector targetWS = AnimVectorToVector( out );
	
	targetWS += boneWS;

	instance[ i_target ] = targetWS;

	return GetTarget( instance );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphPointCloudLookAtSecMotion );

CBehaviorGraphPointCloudLookAtSecMotion::CBehaviorGraphPointCloudLookAtSecMotion()
	: m_maxMasterMotionAngleDeg( 90.f )
	, m_masterBoneAxis( A_X )
	, m_isEnabled( false )
{

}

#ifndef NO_EDITOR

void CBehaviorGraphPointCloudLookAtSecMotion::OnCreatedInEditor()
{
	TBaseClass::OnCreatedInEditor();

	m_masterBones.PushBack( 10 );
	m_masterBones.PushBack( 11 );
	m_masterBones.PushBack( 12 );
}

#endif

void CBehaviorGraphPointCloudLookAtSecMotion::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_animation;
}

void CBehaviorGraphPointCloudLookAtSecMotion::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	CSkeletalAnimationContainer* cont = instance.GetAnimatedComponent()->GetAnimationContainer();
	instance[ i_animation ] = cont->FindAnimationRestricted( m_defaultAnimation );
}

void CBehaviorGraphPointCloudLookAtSecMotion::SampleTransitionPre( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &poseA, const SBehaviorGraphOutput& poseB, Float progress ) const
{
	if ( m_isEnabled )
	{
		const Int32 numBones = (Int32)poseA.m_numBones;
		{
			Float accAngle( 0.f );

			const Uint32 numMasterBones = m_masterBones.Size();
			for ( Uint32 i=0; i<numMasterBones; ++i )
			{
				Int32 masterBone = m_masterBones[ i ];

				const AnimQsTransform& masterBoneA = poseA.m_outputPose[ masterBone ];
				const AnimQsTransform& masterBoneB = poseB.m_outputPose[ masterBone ];

				RedQuaternion restA, restB;
				Float angleRadA( 0.f ), angleRadB( 0.f );
				masterBoneA.Rotation.DecomposeRestAxis( BehaviorUtils::RedVectorFromAxis( m_masterBoneAxis ), restA, angleRadA );
				masterBoneB.Rotation.DecomposeRestAxis( BehaviorUtils::RedVectorFromAxis( m_masterBoneAxis ), restB, angleRadB );

				const Float angleDegA = RAD2DEG( angleRadA );
				const Float angleDegB = RAD2DEG( angleRadB );
				const Float angleDiff = angleDegB - angleDegA;

				accAngle += angleDiff;
			}

			const Float w = Clamp( accAngle / m_maxMasterMotionAngleDeg, -1.f, 1.f );

			//...
		}
	}
}

void CBehaviorGraphPointCloudLookAtSecMotion::SampleTransitionPost( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float progress ) const
{
	if ( m_isEnabled )
	{
		if ( const CSkeletalAnimationSetEntry* animation = instance[ i_animation ] )
		{
			CCacheBehaviorGraphOutput cachePose( context );
			if ( SBehaviorGraphOutput* pose = cachePose.GetPose() )
			{
				ASSERT( animation->GetAnimation()->IsFullyLoaded() );

				const Float time = progress * animation->GetDuration();

				animation->GetAnimation()->Sample( time, pose->m_numBones, pose->m_numFloatTracks, pose->m_outputPose, pose->m_floatTracks );

				BehaviorUtils::BlendingUtils::BlendAdditive( output, output, *pose, 1.f, AT_Ref );
			}
		}
	}

	/*const Float weight = instance[ i_weight ];
	const Float weightAbs = MAbs( weight );
	if ( weightAbs > 0.001f )
	{
		const Float signN = weight;
		const Float signP = -weight;

		const Float factorBicepL = signP * m_bicepBoneL_curve->GetFloatValue( progress );
		const Float factorBicepR = signN * m_bicepBoneL_curve->GetFloatValue( progress );
		const Float factorForearmL = signP * m_bicepBoneL_curve->GetFloatValue( progress );
		const Float factorForearmR = signN * m_bicepBoneL_curve->GetFloatValue( progress );

		AnimQsTransform& bicepBoneL = output.m_outputPose[ m_bicepBoneL ];
		AnimQsTransform& bicepBoneR = output.m_outputPose[ m_bicepBoneR ];
		AnimQsTransform& forearmBoneL = output.m_outputPose[ m_forearmBoneL ];
		AnimQsTransform& forearmBoneR = output.m_outputPose[ m_forearmBoneR ];

		RedQuaternion quatBicepL, quatBicepR, quatForearmL, quatForearmR;
		quatBicepL.SetAxisAngle( BehaviorUtils::RedVectorFromAxis( m_bicepHinge ), factorBicepL * DEG2RAD( m_bicepMaxAngleDeg ) );
		quatBicepR.SetAxisAngle( BehaviorUtils::RedVectorFromAxis( m_bicepHinge ), factorBicepR * DEG2RAD( m_bicepMaxAngleDeg ) );
		quatForearmL.SetAxisAngle( BehaviorUtils::RedVectorFromAxis( m_forearmHinge ), factorForearmL * DEG2RAD( m_forearmMaxAngleDeg ) );
		quatForearmR.SetAxisAngle( BehaviorUtils::RedVectorFromAxis( m_forearmHinge ), factorForearmR * DEG2RAD( m_forearmMaxAngleDeg ) );

		bicepBoneL.Rotation = RedQuaternion::Mul( bicepBoneL.Rotation, quatBicepL );
		bicepBoneR.Rotation = RedQuaternion::Mul( bicepBoneR.Rotation, quatBicepR );
		forearmBoneL.Rotation = RedQuaternion::Mul( forearmBoneL.Rotation, quatForearmL );
		forearmBoneR.Rotation = RedQuaternion::Mul( forearmBoneR.Rotation, quatForearmR );
	}*/
}

#ifdef DEBUG_POINT_CLOUD_LOOK_AT
#pragma optimize("",on)
#endif

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
