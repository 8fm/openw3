
#include "build.h"
#include "behaviorGraphAnimationManualSlot.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "behaviorGraphContext.h"
#include "cacheBehaviorGraphOutput.h"
#include "graphConnectionRebuilder.h"
#include "animatedComponent.h"
#include "skeletalAnimationSet.h"
#include "skeletalAnimationContainer.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "behaviorGraphUtils.inl"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( SRuntimeAnimationData );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationManualSlotNode );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationManualWithInputSlotNode );

SRuntimeAnimationData::SRuntimeAnimationData()
: m_animation( nullptr )
{
}

void SRuntimeAnimationData::SetAnimation( CSkeletalAnimationSetEntry* animation, Bool isActive )
{
	if ( isActive && m_animation && m_animation->GetAnimation() )
	{
		m_animation->GetAnimation()->ReleaseUsage();
	}
	m_animation = animation;
	if ( isActive && m_animation && m_animation->GetAnimation() )
	{
		m_animation->GetAnimation()->AddUsage();
	}
}

void SRuntimeAnimationData::Reset( Bool isActive )
{
	m_state.m_animation = CName::NONE;
	m_state.m_currTime = 0.f;
	m_state.m_prevTime = 0.f;

	m_blendTimer = 0.f;
	m_blendBonesCount = 0;
	SetAnimation( nullptr, isActive );
}

void SRuntimeAnimationData::AutoUpdateTime( Float timeDelta, Bool looped )
{
	m_state.m_prevTime = m_state.m_currTime;
	m_state.m_currTime += timeDelta;

	if ( m_animation && m_state.m_currTime > m_animation->GetDuration() )
	{
		m_state.m_currTime = looped? m_state.m_currTime - m_animation->GetDuration() : m_animation->GetDuration();
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphAnimationManualSlotNode::GetCaption() const
{
	return String::Printf( TXT("Animation manual slot [ %s ]"), m_name.AsChar() );
}

Color CBehaviorGraphAnimationManualSlotNode::GetTitleColor() const
{
	return Color( 255, 64, 64 );
}

void CBehaviorGraphAnimationManualSlotNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Animation ) ) );
}

CName CBehaviorGraphAnimationManualSlotNode::GetAnimationAName( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_animationAData ].m_state.m_animation;
}

CName CBehaviorGraphAnimationManualSlotNode::GetAnimationBName( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_animationBData ].m_state.m_animation;
}

#endif

void CBehaviorGraphAnimationManualSlotNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;

	compiler << i_animationAData;
	compiler << i_animationBData;
	compiler << i_animationDefaultData;

	compiler << i_weight;
	compiler << i_nodeWeight;

	compiler << i_motion;
	compiler << i_motionTrans;
	compiler << i_motionQuat;

	compiler << i_blendOutTime;
	compiler << i_autoUpdateWhenBlendingOut;
}

void CBehaviorGraphAnimationManualSlotNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_timeDelta ] = 0.0f;

	SRuntimeAnimationData& dataA = instance[ i_animationAData ];
	SRuntimeAnimationData& dataB = instance[ i_animationBData ];
	SRuntimeAnimationData& dataDefault = instance[ i_animationDefaultData ];

	dataA.Reset( false );
	dataB.Reset( false );
	dataDefault.Reset( false );

	dataDefault.m_state.m_animation = m_defaultAnimation;
	dataDefault.SetAnimation( FindAnimation( instance, m_defaultAnimation ), false );

	instance[ i_weight ] = 0.f;
	instance[ i_nodeWeight ] = 0.f;

	instance[ i_motion ] = IMT_Anim;
	instance[ i_motionTrans ] = Vector::ZERO_3D_POINT;
	instance[ i_motionQuat ] = Vector::ZERO_3D_POINT;
	instance[ i_blendOutTime ] = -1.0f;
	instance[ i_autoUpdateWhenBlendingOut ] = false;
}

void CBehaviorGraphAnimationManualSlotNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_timeDelta );
	INST_PROP( i_weight );
	INST_PROP( i_nodeWeight );
	INST_PROP( i_animationAData );
	INST_PROP( i_animationBData );
}

void CBehaviorGraphAnimationManualSlotNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	BEH_NODE_UPDATE( ManualSlot );

	instance[ i_timeDelta ] = timeDelta;

	SRuntimeAnimationData& dataA = instance[ i_animationAData ];
	const CSkeletalAnimationSetEntry* animationA = dataA.GetAnimation();
	const CSkeletalAnimation* skAnimationA = animationA ? animationA->GetAnimation() : NULL;

	SRuntimeAnimationData& dataB = instance[ i_animationBData ];
	const CSkeletalAnimationSetEntry* animationB = dataB.GetAnimation();
	const CSkeletalAnimation* skAnimationB = animationB ? animationB->GetAnimation() : NULL;

	// manage auto blend out
	const Float blendOutTime = instance[ i_blendOutTime ];
	if ( blendOutTime >= 0.0f )
	{
		Float & inputWeight = instance[ i_nodeWeight ];
		if ( inputWeight > 0.0f )
		{
			inputWeight = blendOutTime != 0.0f? Max( 0.0f, inputWeight - timeDelta / blendOutTime ) : 0.0f;
			if ( instance[ i_autoUpdateWhenBlendingOut ] )
			{
				dataA.AutoUpdateTime( timeDelta, false );
				dataB.AutoUpdateTime( timeDelta, false );
			}
		}
	}

	const Float inputWeight = ( 1.f - instance[ i_nodeWeight ] );
	ASSERT( inputWeight >= 0.f && inputWeight <= 1.f );

	if ( !skAnimationA || inputWeight > 0.f )
	{
		OnUpdateWhenNoAnimation( instance, context, timeDelta );
	}
}

void CBehaviorGraphAnimationManualSlotNode::OnUpdateWhenNoAnimation( CBehaviorGraphInstance& instance, SBehaviorUpdateContext &context, float timeDelta ) const
{
	SRuntimeAnimationData& dataDefault = instance[ i_animationDefaultData ];
	const CSkeletalAnimationSetEntry* animationDefault = dataDefault.GetAnimation();
	const CSkeletalAnimation* skAnimationDefault = animationDefault ? animationDefault->GetAnimation() : NULL;

	if ( skAnimationDefault )
	{
		dataDefault.AutoUpdateTime( timeDelta, true );
	}
}

void CBehaviorGraphAnimationManualSlotNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	const Float inputWeight = ( 1.f - instance[ i_nodeWeight ] );
	ASSERT( inputWeight >= 0.f && inputWeight <= 1.f );

	const Bool blendWithInput = inputWeight > 0.f;
	if ( blendWithInput )
	{
		SampleWhenNoAnimation( instance, context, output );

		// only if input is not at 100%
		if ( inputWeight < 1.0f )
		{
			CCacheBehaviorGraphOutput cachePoseForSlot( context );
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

	if ( instance[ i_motion ] != IMT_Anim )
	{
		ApplyMotion( instance, output );
	}
}

Bool CBehaviorGraphAnimationManualSlotNode::SamplePoseFromSlot( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &poseForSlot ) const
{
	SRuntimeAnimationData& dataA = instance[ i_animationAData ];
	const CSkeletalAnimationSetEntry* animationA = dataA.GetAnimation();
	const CSkeletalAnimation* skAnimationA = animationA ? animationA->GetAnimation() : NULL;

	SRuntimeAnimationData& dataB = instance[ i_animationBData ];
	const CSkeletalAnimationSetEntry* animationB = dataB.GetAnimation();
	const CSkeletalAnimation* skAnimationB = animationB ? animationB->GetAnimation() : NULL;

	if ( skAnimationA  && skAnimationB )
	{
		CCacheBehaviorGraphOutput cachePose1( context, IsMimic() );
		CCacheBehaviorGraphOutput cachePose2( context, IsMimic() );

		SBehaviorGraphOutput* temp1 = cachePose1.GetPose();
		SBehaviorGraphOutput* temp2 = cachePose2.GetPose();

		if ( temp1 && temp2 )
		{
			const Float weight = instance[ i_weight ];

			SampleAnimation( context, instance, *temp1, animationA, dataA );
			SampleAnimation( context, instance, *temp2, animationB, dataB );

#ifdef DISABLE_SAMPLING_AT_LOD3
			if ( context.GetLodLevel() <= BL_Lod2 )
			{
				poseForSlot.SetInterpolate( *temp1, *temp2, weight );
			}
			else
			{
				poseForSlot.SetInterpolateME( *temp1, *temp2, weight );
			}
#else
			poseForSlot.SetInterpolate( *temp1, *temp2, weight );
#endif
			poseForSlot.MergeEventsAndUsedAnims( *temp1, *temp2, 1.0f - weight, weight );
		}
		return true;
	}
	else if ( skAnimationA )
	{
		SampleAnimation( context, instance, poseForSlot, animationA, dataA );
		return true;
	}
	else
	{
		// no animation
		return false;
	}
}

void CBehaviorGraphAnimationManualSlotNode::SampleWhenNoAnimation( CBehaviorGraphInstance& instance, SBehaviorSampleContext& context, SBehaviorGraphOutput& output ) const
{
	SRuntimeAnimationData& dataDefault = instance[ i_animationDefaultData ];
	const CSkeletalAnimationSetEntry* animationDefault = dataDefault.GetAnimation();
	const CSkeletalAnimation* skAnimationDefault = animationDefault ? animationDefault->GetAnimation() : NULL;

	if ( skAnimationDefault )
	{
		SampleAnimation( context, instance, output, animationDefault, dataDefault );
	}
}

void CBehaviorGraphAnimationManualSlotNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );
	
	Stop( instance );
	ResetMotion( instance );
}

void CBehaviorGraphAnimationManualSlotNode::ApplyMotion( CBehaviorGraphInstance& instance, SBehaviorGraphOutput& output ) const
{
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform motion( hkQsTransform::IDENTITY );
#else
	RedQsTransform motion;
	motion.SetIdentity();
#endif
	const Vector& trans = instance[ i_motionTrans ];
	const Vector& quat = instance[ i_motionQuat ];
#ifdef USE_HAVOK_ANIMATION
	motion.m_translation = TO_CONST_HK_VECTOR_REF( trans );
	motion.m_rotation.m_vec = TO_CONST_HK_VECTOR_REF( quat );
#else
	motion.Translation = reinterpret_cast< const RedVector4& >( trans );
	motion.Rotation.Quat = reinterpret_cast< const RedVector4& >( quat );
#endif
	const Int32 type = instance[ i_motion ];
	ASSERT( type != IMT_Anim );

	if ( type == IMT_Set )
	{
		output.m_deltaReferenceFrameLocal = motion;
	}
	else if ( type == IMT_Blend )
	{
		output.m_deltaReferenceFrameLocal.Slerp( output.m_deltaReferenceFrameLocal, motion, instance[ i_nodeWeight ] );
	}
	else if ( type == IMT_Add )
	{
#ifdef USE_HAVOK_ANIMATION
		output.m_deltaReferenceFrameLocal.setMul( output.m_deltaReferenceFrameLocal, motion );
#else
		output.m_deltaReferenceFrameLocal.SetMul( output.m_deltaReferenceFrameLocal, motion );
#endif
	}
	else
	{
		ASSERT( 0 );
	}
#ifdef RED_ASSERTS_ENABLED
#ifdef USE_HAVOK_ANIMATION
	const Vector& tempVec = TO_CONST_VECTOR_REF( output.m_deltaReferenceFrameLocal.m_translation );
#else
	const Vector& tempVec = reinterpret_cast< const Vector& >( output.m_deltaReferenceFrameLocal.Translation );
#endif
	ASSERT( !Vector::Near2( instance.GetAnimatedComponent()->GetWorldPosition() + 
		tempVec, Vector::ZERO_3D_POINT ) );
#endif
}

void CBehaviorGraphAnimationManualSlotNode::SampleAnimation( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput& output, const CSkeletalAnimationSetEntry* animation, SRuntimeAnimationData& data ) const
{
	const CAnimatedComponent* animatedComponent = instance.GetAnimatedComponent();

	const CSkeletalAnimation* skAnimation = animation->GetAnimation();

	const Float timeDelta = instance[ i_timeDelta ];

#ifdef DISABLE_SAMPLING_AT_LOD3
	if ( context.GetLodLevel() >= BL_Lod3 )
	{
		//..
	}
	else
#endif
	{
		const CSkeleton* skeleton = animatedComponent->GetSkeleton();
		UpdateAndSampleBlendWithCompressedPose( animation, timeDelta, data.m_blendTimer, data.m_state.m_currTime, context, output, skeleton );
	}

	output.Touch();

	PerformPoseCorrection( context, output );

	// Trajectory extraction
	if ( animatedComponent->UseExtractedTrajectory() && animatedComponent->HasTrajectoryBone() )
	{
		output.ExtractTrajectory( animatedComponent );
	}

	// Motion extraction
	if ( skAnimation->HasExtractedMotion() )
	{		
		output.m_deltaReferenceFrameLocal = skAnimation->GetMovementBetweenTime( data.m_state.m_prevTime, data.m_state.m_currTime, 0 );
	}
	else
	{
#ifdef USE_HAVOK_ANIMATION
		output.m_deltaReferenceFrameLocal.setIdentity();
#else
		output.m_deltaReferenceFrameLocal.SetIdentity();
#endif
	}

	const Float eventsAlpha = 1.0f;

	// Gather events from this animation
	animation->GetEventsByTime( data.m_state.m_prevTime, data.m_state.m_currTime, 0, eventsAlpha, NULL, &output );

	output.AppendUsedAnim( SBehaviorUsedAnimationData( animation, data.m_state.m_currTime ) );
}

void CBehaviorGraphAnimationManualSlotNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{

}

void CBehaviorGraphAnimationManualSlotNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{

}

Bool CBehaviorGraphAnimationManualSlotNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	return false;
}

CBehaviorGraph* CBehaviorGraphAnimationManualSlotNode::GetParentGraph()
{
	CBehaviorGraph* graph = GetGraph();
	return graph;
}

CSkeletalAnimationSetEntry* CBehaviorGraphAnimationManualSlotNode::FindAnimation( CBehaviorGraphInstance& instance, const CName& animation ) const
{
	CSkeletalAnimationContainer* cont = instance.GetAnimatedComponent()->GetAnimationContainer();
	return cont->FindAnimation( animation );
}

void CBehaviorGraphAnimationManualSlotNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( instance[ i_animationAData ].GetAnimation() && instance[ i_animationAData ].GetAnimation()->GetAnimation() )
	{
		instance[ i_animationAData ].GetAnimation()->GetAnimation()->AddUsage();
	}
	if ( instance[ i_animationBData ].GetAnimation() && instance[ i_animationBData ].GetAnimation()->GetAnimation() )
	{
		instance[ i_animationBData ].GetAnimation()->GetAnimation()->AddUsage();
	}
}

void CBehaviorGraphAnimationManualSlotNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( instance[ i_animationAData ].GetAnimation() && instance[ i_animationAData ].GetAnimation()->GetAnimation() )
	{
		instance[ i_animationAData ].GetAnimation()->GetAnimation()->ReleaseUsage();
	}
	if ( instance[ i_animationBData ].GetAnimation() && instance[ i_animationBData ].GetAnimation()->GetAnimation() )
	{
		instance[ i_animationBData ].GetAnimation()->GetAnimation()->ReleaseUsage();
	}
}

Bool CBehaviorGraphAnimationManualSlotNode::SetAnimationState( CBehaviorGraphInstance& instance, SRuntimeAnimationData& data, const SAnimationState& inputData ) const
{
	if ( data.m_state.m_animation != inputData.m_animation )
	{
		if ( CSkeletalAnimationSetEntry* anim = FindAnimation( instance, inputData.m_animation ) )
		{
			data.m_state = inputData;
			data.SetAnimation( anim, IsActive( instance ) );
			data.m_blendTimer = 0.f;
		}
		else
		{
			data.Reset( IsActive( instance ) );

			return false;
		}
	}
	else
	{
		data.m_state = inputData;
	}

	return true;
}

Bool CBehaviorGraphAnimationManualSlotNode::PlayAnimation( CBehaviorGraphInstance& instance, const SAnimationState& inputAnimationState, Float weight ) const
{
	ASSERT( weight >= 0.f && weight <= 1.f );

	SRuntimeAnimationData& dataA = instance[ i_animationAData ];
	SRuntimeAnimationData& dataB = instance[ i_animationBData ];

	instance[ i_weight ] = 0.f;
	instance[ i_nodeWeight ] = Clamp( weight, 0.f, 1.f );
	instance[ i_blendOutTime ] = -1.0f;
	instance[ i_autoUpdateWhenBlendingOut ] = false;

	if ( dataB.GetAnimation() )
	{
		dataB.Reset( IsActive( instance ) );
	}

	return SetAnimationState( instance, dataA, inputAnimationState );
}

Bool CBehaviorGraphAnimationManualSlotNode::PlayAnimations( CBehaviorGraphInstance& instance, const SAnimationState& animationA, const SAnimationState& animationB, Float blendWeight, Float weight ) const
{
	ASSERT( weight >= 0.f && weight <= 1.f );
	ASSERT( blendWeight >= 0.f && blendWeight <= 1.f );

	SRuntimeAnimationData& dataA = instance[ i_animationAData ];
	SRuntimeAnimationData& dataB = instance[ i_animationBData ];

	if ( !SetAnimationState( instance, dataA, animationA ) )
	{
		instance[ i_weight ] = 0.f;

		dataB.Reset( IsActive( instance ) );

		return false;
	}

	if ( !SetAnimationState( instance, dataB, animationB ) )
	{
		instance[ i_weight ] = 0.f;

		dataA.Reset( IsActive( instance ) );

		return false;
	}

	instance[ i_weight ] = Clamp( blendWeight, 0.f, 1.f );
	instance[ i_nodeWeight ] = Clamp( weight, 0.f, 1.f );
	instance[ i_blendOutTime ] = -1.0f;
	instance[ i_autoUpdateWhenBlendingOut ] = false;

	ASSERT( dataA.GetAnimation() );
	ASSERT( dataB.GetAnimation() );

	return true;
}

void CBehaviorGraphAnimationManualSlotNode::Stop( CBehaviorGraphInstance& instance ) const
{
	SRuntimeAnimationData& dataA = instance[ i_animationAData ];
	SRuntimeAnimationData& dataB = instance[ i_animationBData ];

	dataA.Reset( IsActive( instance ) );
	dataB.Reset( IsActive( instance ) );

	instance[ i_weight ] = 0.f;
	instance[ i_nodeWeight ] = 0.f;
	instance[ i_blendOutTime ] = -1.0f;
	instance[ i_autoUpdateWhenBlendingOut ] = false;
}

void CBehaviorGraphAnimationManualSlotNode::BlendOut( CBehaviorGraphInstance& instance, Float blendOutTime, Bool continuePlaying ) const
{
	instance[ i_blendOutTime ] = blendOutTime;
	instance[ i_autoUpdateWhenBlendingOut ] = continuePlaying;
	if ( instance[ i_motion ] == IMT_Set )
	{
		instance[ i_motion ] = IMT_Blend;
	}
}

void CBehaviorGraphAnimationManualSlotNode::PerformPoseCorrection( SBehaviorSampleContext& context, SBehaviorGraphOutput& output ) const
{
	// Pose correction
	if ( context.ShouldCorrectPose() )
	{
		context.SetPoseCorrection( output );
	}
}

#ifdef USE_HAVOK_ANIMATION
void CBehaviorGraphAnimationManualSlotNode::AddMotion( CBehaviorGraphInstance& instance, const hkQsTransform& motion ) const
{
	instance[ i_motion ] = IMT_Add;
	instance[ i_motionTrans ] = TO_CONST_VECTOR_REF( motion.m_translation );
	instance[ i_motionQuat ] = TO_CONST_VECTOR_REF( motion.m_rotation.m_vec );
}
#else
void CBehaviorGraphAnimationManualSlotNode::AddMotion( CBehaviorGraphInstance& instance, const RedQsTransform& motion ) const
{
	instance[ i_motion ] = IMT_Add;
	instance[ i_motionTrans ] = reinterpret_cast< const Vector& >( motion.Translation );
	instance[ i_motionQuat ] = reinterpret_cast< const Vector& >( motion.Rotation.Quat );
}
#endif

#ifdef USE_HAVOK_ANIMATION
void CBehaviorGraphAnimationManualSlotNode::SetMotion( CBehaviorGraphInstance& instance, const hkQsTransform& motion ) const
{
	instance[ i_motion ] = IMT_Set;
	instance[ i_motionTrans ] = TO_CONST_VECTOR_REF( motion.m_translation );
	instance[ i_motionQuat ] = TO_CONST_VECTOR_REF( motion.m_rotation.m_vec );
}
#else
void CBehaviorGraphAnimationManualSlotNode::SetMotion( CBehaviorGraphInstance& instance, const RedQsTransform& motion ) const
{
	instance[ i_motion ] = IMT_Set;
	instance[ i_motionTrans ] = reinterpret_cast< const Vector& >( motion.Translation );
	instance[ i_motionQuat ] = reinterpret_cast< const Vector& >( motion.Rotation.Quat );
}
#endif

#ifdef USE_HAVOK_ANIMATION
void CBehaviorGraphAnimationManualSlotNode::BlendMotion( CBehaviorGraphInstance& instance, const hkQsTransform& motion ) const
{
	instance[ i_motion ] = IMT_Blend;
	instance[ i_motionTrans ] = TO_CONST_VECTOR_REF( motion.m_translation );
	instance[ i_motionQuat ] = TO_CONST_VECTOR_REF( motion.m_rotation.m_vec );
}
#else
void CBehaviorGraphAnimationManualSlotNode::BlendMotion( CBehaviorGraphInstance& instance, const RedQsTransform& motion ) const
{
	instance[ i_motion ] = IMT_Blend;
	instance[ i_motionTrans ] = reinterpret_cast< const Vector& >( motion.Translation );
	instance[ i_motionQuat ] = reinterpret_cast< const Vector& >( motion.Rotation.Quat );
}
#endif

void CBehaviorGraphAnimationManualSlotNode::ResetMotion( CBehaviorGraphInstance& instance ) const
{
	instance[ i_motion ] = IMT_Anim;
	instance[ i_motionTrans ] = Vector::ZERO_3D_POINT;
	instance[ i_motionQuat ] = Vector::ZERO_3D_POINT;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CBehaviorGraphAnimationManualSlotNode::CollectAnimationUsageData( const CBehaviorGraphInstance& instance, TDynArray<SBehaviorUsedAnimationData>& collectorArray ) const
{
	const SRuntimeAnimationData& dataA = instance[ i_animationAData ];
	const SRuntimeAnimationData& dataB = instance[ i_animationBData ];
	const Float weight = instance[ i_weight ];

	if (! dataA.m_state.m_animation.Empty())
	{
		SBehaviorUsedAnimationData usageInfo( dataA.GetAnimation(), dataA.m_state.m_currTime, 1.0f - weight);

		collectorArray.PushBack( usageInfo );
	}

	if (! dataB.m_state.m_animation.Empty())
	{
		SBehaviorUsedAnimationData usageInfo( dataB.GetAnimation(), dataB.m_state.m_currTime, weight);

		collectorArray.PushBack( usageInfo );
	}
}
#endif

//////////////////////////////////////////////////////////////////////////

CBehaviorManualSlotInterface::CBehaviorManualSlotInterface()
	: m_slot( NULL )
	, m_instance( NULL )
{

}

CBehaviorManualSlotInterface::~CBehaviorManualSlotInterface()
{

}

void CBehaviorManualSlotInterface::Init( CBehaviorGraphAnimationManualSlotNode* slot, CBehaviorGraphInstance* instance )
{
	m_slot = slot;
	m_instance = instance;
}

void CBehaviorManualSlotInterface::Clear()
{
	m_slot = NULL;
	m_instance = NULL;
}

Bool CBehaviorManualSlotInterface::IsValid() const
{
	return m_slot && m_instance && m_instance->IsBinded();
}

Bool CBehaviorManualSlotInterface::IsActive() const
{
	return IsValid() && m_slot->IsActive( *m_instance );
}

CName CBehaviorManualSlotInterface::GetInstanceName() const
{
	ASSERT( IsValid() );

	return m_instance ? m_instance->GetInstanceName() : CName::NONE;
}

Bool CBehaviorManualSlotInterface::PlayAnimation( const SAnimationState& animation, Float weight )
{
	ASSERT( IsValid() );

	return IsValid() ? m_slot->PlayAnimation( *m_instance, animation, weight ) : false;
}

Bool CBehaviorManualSlotInterface::PlayAnimations( const SAnimationState& animationA, const SAnimationState& animationB, Float blendWeight, Float weight )
{
	ASSERT( IsValid() );

	return IsValid() ? m_slot->PlayAnimations( *m_instance, animationA, animationB, blendWeight, weight ) : false;
}

void CBehaviorManualSlotInterface::Stop()
{
	ASSERT( IsValid() );

	if ( IsValid() )
	{
		m_slot->Stop( *m_instance );
	}
}

void CBehaviorManualSlotInterface::BlendOut( Float blendOutTime, Bool continuePlaying )
{
	ASSERT( IsValid() );

	if ( IsValid() )
	{
		m_slot->BlendOut( *m_instance, blendOutTime, continuePlaying );
	}
}

#ifdef USE_HAVOK_ANIMATION
void CBehaviorManualSlotInterface::SetMotion( const hkQsTransform& motion )
#else
void CBehaviorManualSlotInterface::SetMotion( const RedQsTransform& motion )
#endif
{
	ASSERT( IsValid() );

	if ( IsValid() )
	{	
		m_slot->SetMotion( *m_instance, motion );
	}
}
#ifdef USE_HAVOK_ANIMATION
void CBehaviorManualSlotInterface::BlendMotion( const hkQsTransform& motion )
#else
void CBehaviorManualSlotInterface::BlendMotion( const RedQsTransform& motion )
#endif
{
	ASSERT( IsValid() );

	if ( IsValid() )
	{	
		m_slot->BlendMotion( *m_instance, motion );
	}
}
#ifdef USE_HAVOK_ANIMATION
void CBehaviorManualSlotInterface::AddMotion( const hkQsTransform& motion )
#else
void CBehaviorManualSlotInterface::AddMotion( const RedQsTransform& motion )
#endif
{
	ASSERT( IsValid() );

	if ( IsValid() )
	{
		m_slot->AddMotion( *m_instance, motion );
	}
}

void CBehaviorManualSlotInterface::ResetMotion()
{
	ASSERT( IsValid() );

	if ( IsValid() )
	{
		m_slot->ResetMotion( *m_instance );
	}
}

//////////////////////////////////////////////////////////////////////////

void CBehaviorGraphAnimationManualWithInputSlotNode::OnUpdateWhenNoAnimation( CBehaviorGraphInstance& instance, SBehaviorUpdateContext &context, float timeDelta ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}
}

void CBehaviorGraphAnimationManualWithInputSlotNode::SampleWhenNoAnimation( CBehaviorGraphInstance& instance, SBehaviorSampleContext& context, SBehaviorGraphOutput & output ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Sample( context, instance, output );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphAnimationManualWithInputSlotNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( In ) ) );
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Out ) ) );
}

#endif

void CBehaviorGraphAnimationManualWithInputSlotNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache connections
	m_cachedInputNode = CacheBlock( TXT("In") );
}

void CBehaviorGraphAnimationManualWithInputSlotNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	TBaseClass::GetSyncInfo( instance, info );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphAnimationManualWithInputSlotNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	TBaseClass::SynchronizeTo( instance, info );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->SynchronizeTo( instance, info );
	}
}

void CBehaviorGraphAnimationManualWithInputSlotNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	// just give full alpha - we're in this node, right?
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		// input should be lowered if we're playing something
		const Float nodeWeight = instance[ i_nodeWeight ];
		m_cachedInputNode->ProcessActivationAlpha( instance, ( 1.f - nodeWeight ) * alpha );
	}
}

Bool CBehaviorGraphAnimationManualWithInputSlotNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	Bool eventProcessed = TBaseClass::ProcessEvent( instance, event );

	if ( m_cachedInputNode )
	{
		eventProcessed |= m_cachedInputNode->ProcessEvent( instance, event );
	}

	return eventProcessed ;
}

void CBehaviorGraphAnimationManualWithInputSlotNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}
}

void CBehaviorGraphAnimationManualWithInputSlotNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}
}

Bool CBehaviorGraphAnimationManualWithInputSlotNode::PreloadAnimations( CBehaviorGraphInstance& instance ) const
{
	Bool animationsPreloaded = TBaseClass::PreloadAnimations( instance );

	if ( m_cachedInputNode )
	{
		animationsPreloaded |= m_cachedInputNode->PreloadAnimations( instance );
	}

	return animationsPreloaded;
}
