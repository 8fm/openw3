
#include "build.h"
#include "behaviorGraphAnimationMixerSlot.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphSocket.h"
#include "behaviorGraphContext.h"
#include "cacheBehaviorGraphOutput.h"
#include "../engine/havokAnimationUtils.h"
#include "allocatedBehaviorGraphOutput.h"
#include "behaviorGraphUtils.inl"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/skeletalAnimationContainer.h"
#include "animSyncInfo.h"
#include "animMath.h"
#include "animatedComponent.h"
#include "skeletalAnimationEntry.h"
#include "skeletalAnimation.h"
#include "entity.h"
#include "behaviorProfiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphAnimationMixerSlotNode );
IMPLEMENT_ENGINE_CLASS( SAnimationFullState );
IMPLEMENT_ENGINE_CLASS( SAnimationMappedPose );
IMPLEMENT_RTTI_ENUM( ESAnimationMappedPoseMode );

RED_DEFINE_STATIC_NAME( CExtAnimSoundEvent )

//#define DEBUG_MIXER

#ifdef DEBUG_MIXER
#pragma optimize("",off)
#endif

SAnimationFullState::SAnimationFullState()
{
	m_animation = nullptr;
	Reset();
}

SAnimationFullState::~SAnimationFullState()
{
	SetAnimation( nullptr );
}

SAnimationFullState::SAnimationFullState(SAnimationFullState const & state)
{
	m_animation = nullptr;

	operator=(state);
}

SAnimationFullState & SAnimationFullState::operator = (SAnimationFullState const & state)
{
	m_state = state.m_state;

	SetAnimation( state.m_animation );

	m_blendTimer = state.m_blendTimer;

	m_weight = state.m_weight;
	m_motion = state.m_motion;
	m_fakeMotion = state.m_fakeMotion;
	m_extractTrajectory = state.m_extractTrajectory;
	m_gatherEvents = state.m_gatherEvents;
	m_gatherSyncTokens = state.m_gatherSyncTokens;
	m_muteSoundEvents = state.m_muteSoundEvents;
	m_allowPoseCorrection = state.m_allowPoseCorrection;

	m_additiveType = state.m_additiveType;
	m_convertToAdditive = state.m_convertToAdditive;

	m_bonesIdx = state.m_bonesIdx;
	m_bonesWeight = state.m_bonesWeight;

	m_ID = state.m_ID;

	m_fullEyesWeight = state.m_fullEyesWeight;
	
	return *this;
}

void SAnimationFullState::Reset()
{
	m_state.Reset();

	SetAnimation( nullptr );

	m_weight = 1.f;
	m_motion = true;
	m_fakeMotion = false;
	m_extractTrajectory = true;
	m_gatherEvents = true;
	m_gatherSyncTokens = false;
	m_muteSoundEvents = false;
	m_allowPoseCorrection = true;

	m_additiveType = AT_Local;
	m_convertToAdditive = false;

	m_ID = CGUID::ZERO;

	m_fullEyesWeight = true;
}

void SAnimationFullState::SetAnimation( CSkeletalAnimationSetEntry* animation )
{
	if ( m_animation != animation )
	{
		if ( m_animation && m_animation->GetAnimation() )
		{
			m_animation->GetAnimation()->ReleaseUsage();
		}
		m_animation = animation;
		m_blendTimer = 0.0f;
		if ( m_animation && m_animation->GetAnimation() )
		{
			m_animation->GetAnimation()->AddUsage();
		}
	}
}

CBehaviorGraphAnimationMixerSlotNode::CBehaviorGraphAnimationMixerSlotNode()
	: m_bodyOrMimicMode( true )
	, m_canUseIdles( true )
	, m_postIdleAdditiveType( AT_Local )
	, m_postAllAdditiveType( AT_Local )
#ifndef NO_EDITOR
	, m_debugOverride( false )
#endif
{

}

Bool CBehaviorGraphAnimationMixerSlotNode::IsBodyMode() const
{
	return m_bodyOrMimicMode;
}

Bool CBehaviorGraphAnimationMixerSlotNode::IsMimicsMode() const
{
	return !IsBodyMode();
}

void CBehaviorGraphAnimationMixerSlotNode::CacheConnections()
{
	TBaseClass::CacheConnections();

	if ( IsBodyMode() )
	{
		m_cachedInputNode = CacheBlock( TXT("Input") );
		m_cachedPostIdleNodeA = CacheBlock( TXT("PostIdleA") );
		m_cachedPostIdleNodeB = CacheBlock( TXT("PostIdleB") );
		m_cachedPostAllNodeA = CacheBlock( TXT("PostAllA") );
		m_cachedPostAllNodeB = CacheBlock( TXT("PostAllB") );
	}
	else
	{
		m_cachedInputNode = CacheMimicBlock( TXT("Input") );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphAnimationMixerSlotNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	if ( IsBodyMode() )
	{
		CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
		CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
		CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( PostIdleA ) ) );
		CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( PostIdleB ) ) );
		CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( PostAllA ) ) );
		CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( PostAllB ) ) );
	}
	else
	{
		CreateSocket( CBehaviorGraphMimicAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );	
		CreateSocket( CBehaviorGraphMimicAnimationInputSocketSpawnInfo( CNAME( Input ) ) );
	}
}

String CBehaviorGraphAnimationMixerSlotNode::GetCaption() const
{
	return String::Printf( TXT("Animation mixer slot [ %s ]"), m_name.AsChar() );
}

Color CBehaviorGraphAnimationMixerSlotNode::GetTitleColor() const
{
	return IsBodyMode() ? Color( 255, 64, 64 ) : Color( 128, 0, 128 );
}

#endif

void CBehaviorGraphAnimationMixerSlotNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeDelta;

	compiler << i_animationsData;
	compiler << i_additiveData;
	compiler << i_overrideData;

	compiler << i_cachedPosesNames;
	compiler << i_cachedPosesData;

	compiler << i_mappedPosesIds;
	compiler << i_mappedPosesData;

	compiler << i_idleDataA;
	compiler << i_idleDataB;
	compiler << i_idleDataBlendWeight;

	compiler << i_gatheredSyncTokens;
	compiler << i_syncTokens;

	compiler << i_fullEyesWeightMimicsTracks;
}

void CBehaviorGraphAnimationMixerSlotNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_gatheredSyncTokens ] = false;

	const CSkeleton* s = instance.GetAnimatedComponent()->GetMimicSkeleton();
	if ( s )
	{
		const Uint32 numFullEyesWeightMimicsTracks = m_fullEyesWeightMimicsTracks.Size();
		TDynArray< Int32 >& fullEyesWeightMimicsTracks = instance[ i_fullEyesWeightMimicsTracks ];
		fullEyesWeightMimicsTracks.Reserve( numFullEyesWeightMimicsTracks );
		for ( Uint32 i=0; i<numFullEyesWeightMimicsTracks; ++i )
		{
			const Int32 trackIdx = FindTrackIndex( m_fullEyesWeightMimicsTracks[ i ].AsChar(), s );
			if ( trackIdx != -1 )
			{
				fullEyesWeightMimicsTracks.PushBack( trackIdx );
			}
		}
	}
}

void CBehaviorGraphAnimationMixerSlotNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedPostIdleNodeA )
	{
		m_cachedPostIdleNodeA->Activate( instance );
	}

	if ( m_cachedPostIdleNodeB )
	{
		m_cachedPostIdleNodeB->Activate( instance );
	}

	if ( m_cachedPostAllNodeA )
	{
		m_cachedPostAllNodeA->Activate( instance );
	}

	if ( m_cachedPostAllNodeB )
	{
		m_cachedPostAllNodeB->Activate( instance );
	}
}

void CBehaviorGraphAnimationMixerSlotNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	RemoveAllAnimations( instance );

	if ( m_cachedPostIdleNodeA )
	{
		m_cachedPostIdleNodeA->Deactivate( instance );
	}

	if ( m_cachedPostIdleNodeB )
	{
		m_cachedPostIdleNodeB->Deactivate( instance );
	}

	if ( m_cachedPostAllNodeA )
	{
		m_cachedPostAllNodeA->Deactivate( instance );
	}

	if ( m_cachedPostAllNodeB )
	{
		m_cachedPostAllNodeB->Deactivate( instance );
	}

	TBaseClass::OnDeactivated( instance );
}

void CBehaviorGraphAnimationMixerSlotNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedPostIdleNodeA )
	{
		m_cachedPostIdleNodeA->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedPostIdleNodeB )
	{
		m_cachedPostIdleNodeB->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedPostAllNodeA )
	{
		m_cachedPostAllNodeA->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedPostAllNodeB )
	{
		m_cachedPostAllNodeB->ProcessActivationAlpha( instance, alpha );
	}
}

void CBehaviorGraphAnimationMixerSlotNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_timeDelta );
	INST_PROP( i_animationsData );
	INST_PROP( i_additiveData );
	INST_PROP( i_overrideData );
	INST_PROP( i_cachedPosesNames );
	INST_PROP( i_cachedPosesData );
	INST_PROP( i_idleDataA );
	INST_PROP( i_idleDataB );
	INST_PROP( i_idleDataBlendWeight );
	INST_PROP( i_fullEyesWeightMimicsTracks );
}

void CBehaviorGraphAnimationMixerSlotNode::UpdateIdleAnimation( SAnimationFullState& animationState, Float timeDelta ) const
{
	animationState.m_state.m_prevTime = animationState.m_state.m_currTime;
	animationState.m_state.m_currTime += timeDelta;

	Int32 loops = 0;
	const Float duration = animationState.GetAnimation()->GetDuration();
	while( animationState.m_state.m_currTime >= duration )
	{
		animationState.m_state.m_currTime -= duration;
		++loops;					
	}

	//...
}

void CBehaviorGraphAnimationMixerSlotNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	BEH_NODE_UPDATE( MixerSlot );

	TBaseClass::OnUpdate( context, instance, timeDelta );

	instance[ i_timeDelta ] = timeDelta;

	SAnimationFullState& dataA = instance[ i_idleDataA ];
	SAnimationFullState& dataB = instance[ i_idleDataB ];

	if ( dataA.GetAnimation() )
	{
		UpdateIdleAnimation( dataA, timeDelta );
	}

	if ( dataB.GetAnimation() )
	{
		UpdateIdleAnimation( dataB, timeDelta );
	}

	if ( m_cachedPostIdleNodeA )
	{
		m_cachedPostIdleNodeA->Update( context, instance, timeDelta );
	}

	if ( m_cachedPostIdleNodeB )
	{
		m_cachedPostIdleNodeB->Update( context, instance, timeDelta );
	}

	if ( m_cachedPostAllNodeA )
	{
		m_cachedPostAllNodeA->Update( context, instance, timeDelta );
	}

	if ( m_cachedPostAllNodeB )
	{
		m_cachedPostAllNodeB->Update( context, instance, timeDelta );
	}
}

Bool CBehaviorGraphAnimationMixerSlotNode::HasSomethingToSampled( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_idleDataA ].GetAnimation() || instance[ i_idleDataB ].GetAnimation() 
		|| instance[ i_animationsData ].Size() > 0 || instance[ i_additiveData ].Size() > 0 || instance[ i_overrideData ].Size() > 0
		|| instance[ i_mappedPosesIds ].Size() > 0;
}

void CBehaviorGraphAnimationMixerSlotNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	BEH_NODE_SAMPLE( MixerSlot );

#ifndef NO_EDITOR
	if ( !m_debugOverride )
	{
		if ( !HasSomethingToSampled( instance ) )
		{
			TBaseClass::Sample( context, instance, output );
			return;
		}
	}
#else
	if ( !HasSomethingToSampled( instance ) )
	{
		TBaseClass::Sample( context, instance, output );
		return;
	}
#endif

	const Float timeDelta = instance[ i_timeDelta ];

	const CAnimatedComponent* ac = instance.GetAnimatedComponent();

	CCacheBehaviorGraphOutput cachePose( context, IsMimicsMode() );
	SBehaviorGraphOutput* pose = cachePose.GetPose();
	if ( !pose )
	{
		return;
	}

	// TODO - we don't need to have it always
	CCacheBehaviorGraphOutput cacheConstraintPose( context, IsMimicsMode() );
	SBehaviorGraphOutput* constraintPose = cacheConstraintPose.GetPose();
	if ( !constraintPose )
	{
		return;
	}

	Bool constraintPose_isSampled = false;
	Float constraintPose_weight = 0.f;

	const TDynArray< SAnimationMappedPose >& mappedPosesData = instance[ i_mappedPosesData ];

	// Do we need to sample idle animations? Check base anims weight
	Float baseAnimAccWeight = 0.f;
	{
		TDynArray< SAnimationFullState >& data = instance[ i_animationsData ];
		const Uint32 size = data.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const SAnimationFullState& animData = data[ i ];
			baseAnimAccWeight += animData.m_weight;
		}
	}

	const Bool bodyMode = IsBodyMode();

	// Idle animations
	{
		SAnimationFullState& dataA = instance[ i_idleDataA ];
		SAnimationFullState& dataB = instance[ i_idleDataB ];

		if ( dataA.GetAnimation() )
		{
			if ( dataA.GetAnimation() && dataB.GetAnimation() )
			{
				const Float wAB = instance[ i_idleDataBlendWeight ];
				ASSERT( wAB >= 0.f && wAB <= 1.f );

				//BEH_LOG( TXT("%d; w: %1.3f; A [%s]: %1.3f; B [%s]: %1.3f"), (Int32)(this), w, dataA.m_state.m_animation.AsChar(), dataA.m_state.m_currTime, dataB.m_state.m_animation.AsChar(), dataB.m_state.m_currTime );

				if ( wAB < 0.001f )
				{
					// Only A
					if ( dataA.m_weight < 1.f && !bodyMode )
					{
						TBaseClass::Sample( context, instance, output );

						SampleAnimationWithCorrection( instance, dataA, pose, context, ac, timeDelta );
						
						if ( ShouldSamplePostIdleA() )
						{
							*constraintPose = *pose;
							SamplePostIdleA( context, instance, *constraintPose );
							constraintPose_isSampled = true;
							constraintPose_weight = dataA.m_weight;
						}

						BehaviorUtils::BlendingUtils::BlendAdditive( output, output, *pose, dataA.m_weight, AT_Local );

						output.MergeEventsAndUsedAnims( *pose, dataA.m_weight );
					}
					else
					{
						if ( dataA.m_weight < 1.f )
						{
							TBaseClass::Sample( context, instance, output );

							SampleAnimationWithCorrection( instance, dataA, pose, context, ac, timeDelta );

							output.SetInterpolate( output, *pose, dataA.m_weight );
							output.MergeEventsAndUsedAnims( *pose, dataA.m_weight );
						}
						else
						{
							SampleAnimationWithCorrection( instance, dataA, &output, context, ac, timeDelta );
						}

						if ( ShouldSamplePostIdleA() )
						{
							*constraintPose = output;
							SamplePostIdleA( context, instance, *constraintPose );
							constraintPose_isSampled = true;
							constraintPose_weight = dataA.m_weight;
						}
					}
				}
				else if ( wAB > 0.999f )
				{
					// Only B
					if ( dataB.m_weight < 1.f && !bodyMode )
					{
						TBaseClass::Sample( context, instance, output );

						SampleAnimationWithCorrection( instance, dataB, pose, context, ac, timeDelta );

						if ( ShouldSamplePostIdleB() )
						{
							*constraintPose = *pose;
							SamplePostIdleB( context, instance, *constraintPose );
							constraintPose_isSampled = true;
							constraintPose_weight = dataB.m_weight;
						}

						BehaviorUtils::BlendingUtils::BlendAdditive( output, output, *pose, dataB.m_weight, AT_Local );

						output.MergeEventsAndUsedAnims( *pose, dataB.m_weight );
					}
					else
					{
						if ( dataB.m_weight < 1.f )
						{
							TBaseClass::Sample( context, instance, output );

							SampleAnimationWithCorrection( instance, dataB, pose, context, ac, timeDelta );

							output.SetInterpolate( output, *pose, dataB.m_weight );
							output.MergeEventsAndUsedAnims( *pose, dataB.m_weight );
						}
						else
						{
							SampleAnimationWithCorrection( instance, dataB, &output, context, ac, timeDelta );
						}

						if ( ShouldSamplePostIdleB() )
						{
							*constraintPose = output;
							SamplePostIdleB( context, instance, *constraintPose );
							constraintPose_isSampled = true;
							constraintPose_weight = dataB.m_weight;
						}
					}
				}
				else
				{
					// Blend A and B
					TBaseClass::Sample( context, instance, output );

					const Bool weightSetA = dataA.m_weight > 0.999f;
					const Bool weightSetB = dataB.m_weight > 0.999f;

					if ( bodyMode && ( weightSetA && weightSetB ) )
					{
						SampleAnimationWithCorrection( instance, dataA, &output, context, ac, timeDelta );
						SampleAnimationWithCorrection( instance, dataB, pose, context, ac, timeDelta );

						if ( ShouldSamplePostIdleA() && ShouldSamplePostIdleB() )
						{
							CCacheBehaviorGraphOutput cacheTempPose( context, IsMimicsMode() );
							SBehaviorGraphOutput* poseTemp = cacheTempPose.GetPose();
							if ( poseTemp )
							{
								*constraintPose = output;
								*poseTemp = *pose;
								SamplePostIdleA( context, instance, *constraintPose );
								SamplePostIdleB( context, instance, *poseTemp );
								constraintPose->SetInterpolate( *constraintPose, *poseTemp, wAB );
								constraintPose_isSampled = true;
								constraintPose_weight = 1.f;
							}
						}
						else if ( ShouldSamplePostIdleA() )
						{
							*constraintPose = output;
							SamplePostIdleA( context, instance, *constraintPose );
							constraintPose_isSampled = true;
							constraintPose_weight = wAB;
						}
						else if ( ShouldSamplePostIdleB() )
						{
							*constraintPose = *pose;
							SamplePostIdleB( context, instance, *constraintPose );
							constraintPose_isSampled = true;
							constraintPose_weight = 1.f-wAB;
						}

						output.SetInterpolate( output, *pose, wAB );
						output.MergeEventsAndUsedAnims( *pose, wAB );
					}
					else
					{
						CCacheBehaviorGraphOutput cachePoseB( context, IsMimicsMode() );
						SBehaviorGraphOutput* poseB = cachePoseB.GetPose();
						SBehaviorGraphOutput* poseA = pose;
						if ( poseB )
						{
							const Float weightA = (1.f-wAB) * dataA.m_weight;
							const Float weightB = wAB * dataB.m_weight;

							SampleAnimationWithCorrection( instance, dataA, poseA, context, ac, timeDelta );
							SampleAnimationWithCorrection( instance, dataB, poseB, context, ac, timeDelta );

							BehaviorUtils::BlendingUtils::BlendAdditive( output, output, *poseA, weightA, AT_Local );
							BehaviorUtils::BlendingUtils::BlendAdditive( output, output, *poseB, weightB, AT_Local );

							output.MergeEventsAndUsedAnims( *poseA, *poseB, weightA, weightB );

							if ( ShouldSamplePostIdleA() && ShouldSamplePostIdleB() )
							{
								CCacheBehaviorGraphOutput cacheTempPose( context, IsMimicsMode() );
								SBehaviorGraphOutput* poseTemp = cacheTempPose.GetPose();
								if ( poseTemp )
								{
									*constraintPose = *poseA;
									*poseTemp = *poseB;
									SamplePostIdleA( context, instance, *constraintPose );
									SamplePostIdleB( context, instance, *poseTemp );

									if ( dataA.m_weight < 1.f || dataB.m_weight < 1.f )
									{
										poseB->SetIdentity();

										if ( dataA.m_weight < 1.f )
										{
											constraintPose->SetInterpolate( *poseB, *constraintPose, dataA.m_weight );
										}
										if ( dataB.m_weight < 1.f )
										{
											poseTemp->SetInterpolate( *poseB, *poseTemp, dataB.m_weight );
										}
									}

									constraintPose->SetInterpolate( *constraintPose, *poseTemp, wAB );
									constraintPose_isSampled = true;
								}
							}
							else if ( ShouldSamplePostIdleA() )
							{
								*constraintPose = *poseA;
								SamplePostIdleA( context, instance, *constraintPose );
								constraintPose_isSampled = true;
								constraintPose_weight = weightA;
							}
							else if ( ShouldSamplePostIdleB() )
							{
								*constraintPose = *poseB;
								SamplePostIdleB( context, instance, *constraintPose );
								constraintPose_isSampled = true;
								constraintPose_weight = weightB;
							}
						}
					}
				}
			}
			else
			{
				const Float wAB = instance[ i_idleDataBlendWeight ];
				ASSERT( wAB >= 0.f && wAB <= 1.f );

				const Float weight = (1.f-wAB) * dataA.m_weight;

				if ( weight < 1.f )
				{
					TBaseClass::Sample( context, instance, output );

					SampleAnimationWithCorrection( instance, dataA, pose, context, ac, timeDelta );
					if ( ShouldSamplePostIdleA() )
					{
						*constraintPose = *pose;
						SamplePostIdleA( context, instance, *constraintPose );
						constraintPose_isSampled = true;
						constraintPose_weight = weight;
					}

					output.SetInterpolate( output, *pose, weight );
					output.MergeEventsAndUsedAnims( *pose, weight );
				}
				else
				{
					SampleAnimationWithCorrection( instance, dataA, &output, context, ac, timeDelta );
					if ( ShouldSamplePostIdleA() )
					{
						*constraintPose = output;
						SamplePostIdleA( context, instance, *constraintPose );
						constraintPose_isSampled = true;
						constraintPose_weight = 1.f;
					}
				}
			}
		}
		else if ( dataB.GetAnimation() )
		{
			const Float wAB = instance[ i_idleDataBlendWeight ];
			ASSERT( wAB >= 0.f && wAB <= 1.f );

			const Float weight = wAB * dataB.m_weight;

			if ( weight < 1.f )
			{
				TBaseClass::Sample( context, instance, output );
				
				SampleAnimationWithCorrection( instance, dataB, pose, context, ac, timeDelta );
				if ( ShouldSamplePostIdleB() )
				{
					*constraintPose = *pose;
					SamplePostIdleB( context, instance, *constraintPose );
					constraintPose_isSampled = true;
					constraintPose_weight = weight;
				}

				output.SetInterpolate( output, *pose, weight );
				output.MergeEventsAndUsedAnims( *pose, weight );
			}
			else
			{
				SampleAnimationWithCorrection( instance, dataB, &output, context, ac, timeDelta );
				if ( ShouldSamplePostIdleB() )
				{
					*constraintPose = output;
					SamplePostIdleB( context, instance, *constraintPose );
					constraintPose_isSampled = true;
					constraintPose_weight = 1.f;
				}
			}
		}
		else if ( baseAnimAccWeight < 1.f || !bodyMode )
		{
			TBaseClass::Sample( context, instance, output );
		}
	}

	// Base animations
	{
		TDynArray< SAnimationFullState >& data = instance[ i_animationsData ];
		if ( data.Size() > 0 && baseAnimAccWeight > 0.f )
		{
			CCacheBehaviorGraphOutput cacheAccPose( context, IsMimicsMode() );
			SBehaviorGraphOutput* accPose = cacheAccPose.GetPose();
			if ( accPose )
			{
				if ( bodyMode )
				{
					BehaviorUtils::BlendingUtils::SetPoseZero( *accPose );

					// Clips
					const Uint32 size = data.Size();
					for ( Uint32 i=0; i<size; ++i )
					{
						SAnimationFullState& animData = data[ i ];
						if ( animData.GetAnimation() )
						{
							SampleAnimationWithCorrection( instance, animData, pose, context, ac, timeDelta );

							if ( animData.m_gatherSyncTokens )
							{
								SyncTokens( instance, animData, baseAnimAccWeight );
							}
						}

						BehaviorUtils::BlendingUtils::BlendPosesNormal( *accPose, *pose, animData.m_weight );
						accPose->MergeEventsAndUsedAnims( *pose, animData.m_weight/baseAnimAccWeight );
					}

					BehaviorUtils::BlendingUtils::RenormalizePose( *accPose, baseAnimAccWeight );

					if ( baseAnimAccWeight < 1.f )
					{
						output.SetInterpolate( output, *accPose, baseAnimAccWeight );
						output.MergeEventsAndUsedAnims( *accPose, baseAnimAccWeight );
					}
					else
					{
						output = *accPose;
					}
				}
				else
				{
					//BehaviorUtils::BlendingUtils::SetPoseIdentity( *accPose );

					// Clips
					const Uint32 size = data.Size();
					for ( Uint32 i=0; i<size; ++i )
					{
						SAnimationFullState& animData = data[ i ];

						if ( animData.GetAnimation() )
						{
							SampleAnimationWithCorrection( instance, animData, pose, context, ac, timeDelta );

							BehaviorUtils::BlendingUtils::BlendAdditive( output, output, *pose, animData.m_weight, AT_Local );

							if ( animData.m_fullEyesWeight && animData.m_weight < 1.f )
							{
								const Float weightInv = 1.f - animData.m_weight;
								const Int32 numOutputTracks = (Int32)output.m_numFloatTracks;

								const TDynArray< Int32 >& fullEyesWeightMimicsTracks = instance[ i_fullEyesWeightMimicsTracks ];
								const Uint32 numEyesTracks = fullEyesWeightMimicsTracks.Size();
								for ( Uint32 j=0; j<numEyesTracks; ++j )
								{
									const Int32 trackIdx = fullEyesWeightMimicsTracks[ j ];
									ASSERT( trackIdx != -1 );
									ASSERT( trackIdx < numOutputTracks );

									if ( trackIdx != -1 && trackIdx < numOutputTracks )
									{
										output.m_floatTracks[ trackIdx ] += weightInv * pose->m_floatTracks[ trackIdx ];
									}
								}
							}

							output.MergeEventsAndUsedAnims( *pose, animData.m_weight );
						}
					}

					BehaviorUtils::BlendingUtils::RenormalizePoseRotations( output );
				}
			}
		}
	}

	// Override
	{
		TDynArray< SAnimationFullState >& data = instance[ i_overrideData ];
		if ( data.Size() > 0 )
		{
			const Uint32 size = data.Size();
			for ( Uint32 i=0; i<size; ++i )
			{
				SAnimationFullState& animData = data[ i ];
				if ( animData.GetAnimation() )
				{
					SampleAnimationWithCorrection( instance, animData, pose, context, ac, timeDelta );
				}

				BehaviorUtils::BlendingUtils::BlendPartialPosesOverride( output.m_outputPose, pose->m_outputPose, pose->m_numBones, animData.m_weight, animData.m_bonesIdx, animData.m_bonesWeight );
			}
		}
	}

	// Additive constraints. All constraints are calculated based on idle poses
	{
		if ( constraintPose_isSampled )
		{
			BehaviorUtils::BlendingUtils::BlendAdditive( output, output, *constraintPose, constraintPose_weight, m_postIdleAdditiveType );
		}
	}

	// Post all pass
	{
		const Bool shouldSamplePostAllA = ShouldSamplePostAllA();
		const Bool shouldSamplePostAllB = ShouldSamplePostAllB();

		if ( shouldSamplePostAllA || shouldSamplePostAllB )
		{
			const SAnimationFullState& dataA = instance[ i_idleDataA ];
			const SAnimationFullState& dataB = instance[ i_idleDataB ];

			const Float wAB = instance[ i_idleDataBlendWeight ];
			const Float weightA = dataA.m_weight * (1.f-wAB);
			const Float weightB = dataB.m_weight * wAB;

			SBehaviorGraphOutput* poseA = pose;
			SBehaviorGraphOutput* poseB = constraintPose;

			Bool sampleA = shouldSamplePostAllA && dataA.GetAnimation() && weightA > 0.f;
			const Bool sampleB = shouldSamplePostAllB && dataB.GetAnimation() && weightB > 0.f;

#ifndef NO_EDITOR
			if ( m_debugOverride )
			{
				sampleA = true;
			}
#endif

			if ( sampleA )
			{
				*poseA = output;
				SamplePostAllA( context, instance, *poseA );
			}
			if ( sampleB )
			{
				*poseB = output;
				SamplePostAllB( context, instance, *poseB );
			}

			if ( sampleA )
			{
				BehaviorUtils::BlendingUtils::BlendAdditive( output, output, *poseA, weightA, m_postAllAdditiveType );
			}
			if ( sampleB )
			{
				BehaviorUtils::BlendingUtils::BlendAdditive( output, output, *poseB, weightB, m_postAllAdditiveType );
			}
		}
	}

	// Additives
	{
		Bool usedCachedAdditivePoses[ 32 ];
		Red::System::MemoryZero( &usedCachedAdditivePoses, sizeof( usedCachedAdditivePoses ) );

		TDynArray< SAnimationFullState >& data = instance[ i_additiveData ];
		if ( data.Size() > 0 )
		{
			BehaviorUtils::BlendingUtils::SetPoseIdentity( pose->m_outputPose, pose->m_numBones );

			const Uint32 size = data.Size();
			for ( Uint32 i=0; i<size; ++i )
			{
				const SAnimationFullState& animData = data[ i ];
				if ( animData.GetAnimation() )
				{
					//SampleAnimation( animData, pose, context, ac, timeDelta );
					{
						animData.GetAnimation()->GetAnimation()->Sample( animData.m_state.m_currTime, pose->m_numBones, pose->m_numFloatTracks, pose->m_outputPose, pose->m_floatTracks );
						
						// Events
						if ( animData.m_gatherEvents )
						{
							CName tag = CName::NONE;
							Int32 sampleRange( -1 );
							if ( const CEntity* parent = ac->GetEntity() )
							{
								tag = parent->GetSfxTag();
#ifdef NO_EDITOR
								sampleRange = animData.GetAnimation()->FindEventGroupRangeIndex( tag );
#endif
							}
							if( !animData.m_muteSoundEvents )
							{
								animData.GetAnimation()->GetEventsByTime( animData.m_state.m_prevTime, animData.m_state.m_currTime, 0, animData.m_weight, NULL, &output, tag, sampleRange );
							}
							else
							{
								animData.GetAnimation()->GetEventsByTime( animData.m_state.m_prevTime, animData.m_state.m_currTime, 0, animData.m_weight, NULL, pose, tag, sampleRange );
								MuteSoundEvents( *pose);
								output.MergeEvents( *pose );
							}
						}

						if ( pose->m_numBones > 0 )
						{
							if ( animData.m_extractTrajectory && ac->UseExtractedTrajectory() && ac->HasTrajectoryBone() )
							{
								pose->ExtractTrajectory( ac );
							}
							else
							{
								pose->m_outputPose[ 0 ].Rotation.Normalize();
							}
						}

						if ( animData.m_fakeMotion && animData.GetAnimation()->GetAnimation()->HasExtractedMotion() )
						{
							AnimQsTransform motion = animData.GetAnimation()->GetAnimation()->GetMovementBetweenTime( 0.f, animData.m_state.m_currTime, 0 );
							pose->m_outputPose[ 0 ].SetMul( pose->m_outputPose[ 0 ], motion );
						}
					}
				}

				if ( animData.m_convertToAdditive )
				{
					Int32 index = FindCachedPose( instance, animData.m_state.m_animation );
					if ( index == -1 )
					{
						index = AddCachedPose( instance, animData );
					}

					if ( index != -1 )
					{
						SBehaviorGraphOutput* firstFrame = GetCachedPose( instance, index );
						if ( firstFrame )
						{
							ImportAnimationUtils::ConvertToAdditiveFrame( firstFrame->m_outputPose, pose->m_outputPose, pose->m_numBones, animData.m_additiveType );
						}

						usedCachedAdditivePoses[ index ] = true;
					}
					else
					{
						continue;
					}
				}

				if ( animData.m_bonesIdx.Size() > 0 )
				{
					FilterAdditiveAnimation( animData, *pose );
				}

				BehaviorUtils::BlendingUtils::BlendAdditive( output, output, *pose, animData.m_weight, animData.m_additiveType );
			}
		}

		if ( HasAnyCachedPose( instance ) )
		{
			TDynArray< CName >& cachedPosesNames = instance[ i_cachedPosesNames ];
			TDynArray< CAllocatedBehaviorGraphOutput >& cachedPosesData = instance[ i_cachedPosesData ];

			ASSERT( cachedPosesData.SizeInt() <= ARRAY_COUNT( usedCachedAdditivePoses ) );

			for ( Int32 i=cachedPosesData.SizeInt()-1; i>=0; --i )
			{
				if ( !usedCachedAdditivePoses[ i ] )
				{
					cachedPosesData[ i ].Free( instance );

					cachedPosesData.RemoveAt( i );
					cachedPosesNames.RemoveAt( i );
				}
			}

			ASSERT( cachedPosesData.Size() == cachedPosesNames.Size() );
		}
	}

	// Additive all poses
	{
		const Uint32 size = mappedPosesData.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const SAnimationMappedPose& pose = mappedPosesData[ i ];
			if ( pose.m_mode == AMPM_Additive && pose.m_weight > 0.f && pose.m_correctionID.IsZero() && !pose.m_correctionIdleID )
			{
				BlendAdditiveMappedPose( output, pose, 1.f );
			}
		}
	}

	// Override all poses
	{
		const Uint32 size = mappedPosesData.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const SAnimationMappedPose& pose = mappedPosesData[ i ];
			if ( pose.m_mode == AMPM_Override && pose.m_weight > 0.f && pose.m_correctionID.IsZero() && !pose.m_correctionIdleID )
			{
				BlendOverrideMappedPose( output, pose, 1.f );
			}
		}
	}

}

void CBehaviorGraphAnimationMixerSlotNode::GatherSyncTokens( CBehaviorGraphInstance& instance ) const
{
	const CEntity* entity = instance.GetAnimatedComponent()->GetEntity();

	TDynArray< CAnimationSyncToken* >& tokens = instance[ i_syncTokens ];
	entity->OnCollectAnimationSyncTokens( CName::NONE, tokens );
}

void CBehaviorGraphAnimationMixerSlotNode::SyncTokens( CBehaviorGraphInstance& instance, const SAnimationFullState& animData, Float accWeight ) const
{
	ASSERT( accWeight > 0.f );

	if ( !instance[ i_gatheredSyncTokens ] )
	{
		GatherSyncTokens( instance );

		instance[ i_gatheredSyncTokens ] = true;
	}

	const TDynArray< CAnimationSyncToken* >& tokens = instance[ i_syncTokens ];
	const Uint32 num = tokens.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		CSyncInfo syncInfo;
		syncInfo.m_prevTime = animData.m_state.m_prevTime;
		syncInfo.m_currTime = animData.m_state.m_currTime;

		tokens[i]->Sync( animData.m_state.m_animation, syncInfo, animData.m_weight/accWeight );
	}
}

void CBehaviorGraphAnimationMixerSlotNode::ClearSyncTokens( CBehaviorGraphInstance& instance ) const
{
	TDynArray< CAnimationSyncToken* >& tokens = instance[ i_syncTokens ];
	for ( Uint32 i=0; i<tokens.Size(); ++i )
	{
		tokens[i]->Reset();
		delete tokens[i];
	}
	tokens.Clear();
}

void CBehaviorGraphAnimationMixerSlotNode::MuteSoundEvents( SBehaviorGraphOutput& pose) const
{
	static CClass* animSoundEventClass = SRTTI::GetInstance().FindClass( CNAME( CExtAnimSoundEvent ) );
	for ( Uint32 i=0; i<pose.m_numEventsFired; ++i )
	{
		if ( pose.m_eventsFired[ i ].m_extEvent->GetClass()->IsA( animSoundEventClass ))
		{
			pose.m_eventsFired[ i ].m_alpha = 0.f;
		}
	}
}


Bool CBehaviorGraphAnimationMixerSlotNode::IsPoseForIdle( CBehaviorGraphInstance& instance, const SAnimationMappedPose& pose, Float& idleWeight ) const
{
	const SAnimationFullState& dataA = instance[ i_idleDataA ];
	const SAnimationFullState& dataB = instance[ i_idleDataB ];

	const Float weightAB = instance[ i_idleDataBlendWeight ];

	if ( dataB.GetAnimation() && pose.m_correctionIdleID == dataB.m_state.m_animation )
	{
		idleWeight = weightAB;

		return true;
	}
	else if ( dataA.GetAnimation() && pose.m_correctionIdleID == dataA.m_state.m_animation )
	{
		idleWeight = 1.f-weightAB;

		return true;
	}
	else
	{
		return false;
	}
}

void CBehaviorGraphAnimationMixerSlotNode::BlendAdditiveMappedPose( SBehaviorGraphOutput &output, const SAnimationMappedPose& pose, Float parentWeight ) const
{
	const Float poseWeight = pose.m_weight * parentWeight;

	// Bones
	const Int32 poseNumBones = (Int32)output.m_numBones;
	const Uint32 numBones = ( pose.m_bonesMapping.Size(), pose.m_bones.Size() );
	if ( poseWeight < 1.f )
	{
		for ( Uint32 i=0; i<numBones; ++i )
		{
			const Int32 boneIndex = pose.m_bonesMapping[ i ];

			if ( boneIndex != -1 && boneIndex < poseNumBones )
			{
				const EngineQsTransform& boneTrans = pose.m_bones[ i ];
				const AnimQsTransform& boneQsTrans = ENGINE_QS_TRANSFORM_TO_CONST_ANIM_QS_TRANSFORM_REF( boneTrans );

				AnimQsTransform boneToAddFinal;
				boneToAddFinal.Lerp( AnimQsTransform::IDENTITY, boneQsTrans, poseWeight );

				AnimQsTransform& bone = output.m_outputPose[ boneIndex ];
				bone.SetMul( bone, boneToAddFinal );
			}
		}
	}
	else
	{
		for ( Uint32 i=0; i<numBones; ++i )
		{
			const Int32 boneIndex = pose.m_bonesMapping[ i ];

			if ( boneIndex != -1 && boneIndex < poseNumBones )
			{
				const EngineQsTransform& boneTrans = pose.m_bones[ i ];
				const AnimQsTransform& boneQsTrans = ENGINE_QS_TRANSFORM_TO_CONST_ANIM_QS_TRANSFORM_REF( boneTrans );

				AnimQsTransform& bone = output.m_outputPose[ boneIndex ];
				bone.SetMul( bone, boneQsTrans );
			}
		}
	}

	// Tracks
	const Int32 poseNumTracks = (Int32)output.m_numFloatTracks;
	if ( pose.m_tracksMapping.Size() > 0 )
	{
		const Uint32 numTracks = Min( pose.m_tracksMapping.Size(), pose.m_tracks.Size() );
		for ( Uint32 i=0; i<numTracks; ++i )
		{
			const Int32 trackIndex = pose.m_tracksMapping[ i ];

			if ( trackIndex != -1 && trackIndex < poseNumTracks )
			{
				output.m_floatTracks[ trackIndex ] += poseWeight * pose.m_tracks[ i ];
			}
		}
	}
	else if ( pose.m_tracksMapping.Size() == 0 && pose.m_tracks.Size() > 0 && pose.m_tracks.SizeInt() == poseNumTracks )
	{
		for ( Int32 i=0; i<poseNumTracks; ++i )
		{
			output.m_floatTracks[ i ] += poseWeight * pose.m_tracks[ i ];
		}
	}
}

void CBehaviorGraphAnimationMixerSlotNode::BlendOverrideMappedPose( SBehaviorGraphOutput &output, const SAnimationMappedPose& pose, Float parentWeight ) const
{
	const Float poseWeight = pose.m_weight * parentWeight;

	// Bones
	const Int32 poseNumBones = (Int32)output.m_numBones;
	const Uint32 numBones = ( pose.m_bonesMapping.Size(), pose.m_bones.Size() );
	if ( poseWeight < 1.f )
	{
		for ( Uint32 i=0; i<numBones; ++i )
		{
			const Int32 boneIndex = pose.m_bonesMapping[ i ];

			if ( boneIndex != -1 && boneIndex < poseNumBones )
			{
				const EngineQsTransform& boneTrans = pose.m_bones[ i ];
				const AnimQsTransform& boneQsTrans = ENGINE_QS_TRANSFORM_TO_CONST_ANIM_QS_TRANSFORM_REF( boneTrans );

				AnimQsTransform& bone = output.m_outputPose[ boneIndex ];
				bone.Lerp( bone, boneQsTrans, poseWeight );
			}
		}
	}
	else
	{
		for ( Uint32 i=0; i<numBones; ++i )
		{
			const Int32 boneIndex = pose.m_bonesMapping[ i ];

			if ( boneIndex != -1 && boneIndex < poseNumBones )
			{
				const EngineQsTransform& boneTrans = pose.m_bones[ i ];
				const AnimQsTransform& boneQsTrans = ENGINE_QS_TRANSFORM_TO_CONST_ANIM_QS_TRANSFORM_REF( boneTrans );

				AnimQsTransform& bone = output.m_outputPose[ boneIndex ];
				bone = boneQsTrans;
			}
		}
	}

	// Tracks
	const Int32 poseNumTracks = (Int32)output.m_numFloatTracks;
	if ( pose.m_tracksMapping.Size() > 0 )
	{
		const Uint32 numTracks = Min( pose.m_tracksMapping.Size(), pose.m_tracks.Size() );
		for ( Uint32 i=0; i<numTracks; ++i )
		{
			const Int32 trackIndex = pose.m_tracksMapping[ i ];

			if ( trackIndex != -1 && trackIndex < poseNumTracks )
			{
				output.m_floatTracks[ trackIndex ] = poseWeight * pose.m_tracks[ i ];
			}
		}
	}
	else if ( pose.m_tracksMapping.Size() == 0 && pose.m_tracks.Size() > 0 && pose.m_tracks.SizeInt() == poseNumTracks )
	{
		for ( Int32 i=0; i<poseNumTracks; ++i )
		{
			output.m_floatTracks[ i ] = poseWeight * pose.m_tracks[ i ];
		}
	}
}

void CBehaviorGraphAnimationMixerSlotNode::SampleAnimation( SAnimationFullState& animData, SBehaviorGraphOutput* pose, SBehaviorSampleContext& context, const CAnimatedComponent* ac, Float timeDelta ) const
{
	ASSERT( animData.GetAnimation() );
	ASSERT( ac );
	ASSERT( ac->GetSkeleton() );
	ASSERT( pose );

	UpdateAndSampleBlendWithCompressedPose( animData.GetAnimation(), timeDelta, animData.m_blendTimer, animData.m_state.m_currTime, context, *pose, ac->GetSkeleton() );

	if ( IsBodyMode() )
	{
		if ( context.ShouldCorrectPose() )
		{
			context.SetPoseCorrection( *pose );
		}

		// Trajectory extraction
		if ( animData.m_extractTrajectory && ac->UseExtractedTrajectory() && ac->HasTrajectoryBone() )
		{
			pose->ExtractTrajectory( ac );
		}
		else if ( pose->m_numBones > 0 )
		{
#ifdef USE_HAVOK_ANIMATION
			pose->m_outputPose[ 0 ].m_rotation.normalize();
#else
			pose->m_outputPose[ 0 ].Rotation.Normalize();
#endif
		}
	}

	// Events
	if ( animData.m_gatherEvents )
	{
		CName tag = CName::NONE;
		Int32 sampleRange( -1 );
		if ( const CEntity* parent = ac->GetEntity() )
		{
			tag = parent->GetSfxTag();
#ifdef NO_EDITOR
			sampleRange = animData.GetAnimation()->FindEventGroupRangeIndex( tag );
#endif
		}

		animData.GetAnimation()->GetEventsByTime( animData.m_state.m_prevTime, animData.m_state.m_currTime, 0, animData.m_weight, NULL, pose, tag, sampleRange );
		if(animData.m_muteSoundEvents)
		{
			MuteSoundEvents( *pose);
		}

	}

	// Motion extraction
	if ( IsBodyMode() && animData.GetAnimation()->GetAnimation()->HasExtractedMotion() )
	{	
		// TODO - loop always is 0, maybe if prevTime > currTime loop should be 1?
		if ( animData.m_motion )
		{
			pose->m_deltaReferenceFrameLocal = animData.GetAnimation()->GetAnimation()->GetMovementBetweenTime( animData.m_state.m_prevTime, animData.m_state.m_currTime, 0 );
		}
		else if ( animData.m_fakeMotion && pose->m_numBones > 0 )
		{
			AnimQsTransform motion = animData.GetAnimation()->GetAnimation()->GetMovementBetweenTime( 0.f, animData.m_state.m_currTime, 0 );
			pose->m_outputPose[ 0 ].SetMul( pose->m_outputPose[ 0 ], motion );
		}
	}
	else
	{
#ifdef USE_HAVOK_ANIMATION
		pose->m_deltaReferenceFrameLocal.setIdentity();
#else
		pose->m_deltaReferenceFrameLocal.SetIdentity();
#endif
	}
}

void CBehaviorGraphAnimationMixerSlotNode::SampleAnimationWithCorrection( CBehaviorGraphInstance& instance, SAnimationFullState& animData, SBehaviorGraphOutput* pose, SBehaviorSampleContext& context, const CAnimatedComponent* ac, Float timeDelta ) const
{
	SampleAnimation( animData, pose, context, ac, timeDelta );

	// Correction
	{
		TDynArray< SAnimationMappedPose >& mappedPosesData = instance[ i_mappedPosesData ];

		const Uint32 size = mappedPosesData.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const SAnimationMappedPose& mappedPose = mappedPosesData[ i ];

			if ( mappedPose.m_weight > 0.f )
			{
				Float idleWeight = 1.f;

				const Bool thisID = mappedPose.m_correctionID == animData.m_ID;
				const Bool idleID = animData.m_allowPoseCorrection && IsPoseForIdle( instance, mappedPose, idleWeight );

				if ( thisID || idleID )
				{
					if ( mappedPose.m_mode == AMPM_Additive )
					{
						BlendAdditiveMappedPose( *pose, mappedPose, idleWeight );
					}
					else if ( mappedPose.m_mode == AMPM_Override )
					{
						BlendOverrideMappedPose( *pose, mappedPose, idleWeight );
					}
					else
					{
						ASSERT( 0 );
					}
				}
			}
		}
	}
}

Bool CBehaviorGraphAnimationMixerSlotNode::ShouldSamplePostIdleA() const
{
	return m_cachedPostIdleNodeA != nullptr;
}

Bool CBehaviorGraphAnimationMixerSlotNode::ShouldSamplePostIdleB() const
{
	return m_cachedPostIdleNodeB != nullptr;
}

void CBehaviorGraphAnimationMixerSlotNode::SamplePostIdleA( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	ASSERT( m_cachedPostIdleNodeA );
	m_cachedPostIdleNodeA->Sample( context, instance, output );
}

void CBehaviorGraphAnimationMixerSlotNode::SamplePostIdleB( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	ASSERT( m_cachedPostIdleNodeB );
	m_cachedPostIdleNodeB->Sample( context, instance, output );
}

Bool CBehaviorGraphAnimationMixerSlotNode::ShouldSamplePostAllA() const
{
	return m_cachedPostAllNodeA != nullptr;
}

Bool CBehaviorGraphAnimationMixerSlotNode::ShouldSamplePostAllB() const
{
	return m_cachedPostAllNodeB != nullptr;
}

void CBehaviorGraphAnimationMixerSlotNode::SamplePostAllA( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	ASSERT( m_cachedPostAllNodeA );
	m_cachedPostAllNodeA->Sample( context, instance, output );
}

void CBehaviorGraphAnimationMixerSlotNode::SamplePostAllB( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	ASSERT( m_cachedPostAllNodeB );
	m_cachedPostAllNodeB->Sample( context, instance, output );
}

void CBehaviorGraphAnimationMixerSlotNode::FilterAdditiveAnimation( const SAnimationFullState& animation, SBehaviorGraphOutput& pose ) const
{
	ASSERT( animation.m_bonesIdx.Size() > 0 );
	ASSERT( animation.m_bonesIdx.Size() == animation.m_bonesWeight.Size() );

	const Int32 numBones = (Int32)pose.m_numBones;
	const Int32 numFilter = animation.m_bonesIdx.SizeInt();

	Int32 lastBone = 0;
	for ( Int32 f=0; f<numFilter; ++f )
	{
		const Int32 bIdx = animation.m_bonesIdx[ f ];
		const Float wIdx = animation.m_bonesWeight[ f ];

		for ( Int32 l=lastBone; l<bIdx; ++l )
		{
			ASSERT( l != -1 && l < (Int32)pose.m_numBones );
			pose.m_outputPose[ l ].SetIdentity();
		}
		lastBone = bIdx+1;

		if ( wIdx < 1.f )
		{
			ASSERT( bIdx != -1 && bIdx < (Int32)pose.m_numBones );
			pose.m_outputPose[ bIdx ].Slerp( RedQsTransform::IDENTITY, pose.m_outputPose[ bIdx ], wIdx );
		}
	}
	for ( Int32 l=lastBone; l<numBones; ++l )
	{
		ASSERT( l != -1 && l < (Int32)pose.m_numBones );
		pose.m_outputPose[ l ].SetIdentity();
	}
}

CBehaviorGraph* CBehaviorGraphAnimationMixerSlotNode::GetParentGraph()
{
	CBehaviorGraph* graph = GetGraph();
	return graph;
}

CSkeletalAnimationSetEntry* CBehaviorGraphAnimationMixerSlotNode::FindAnimation( CBehaviorGraphInstance& instance, const CName& animation ) const
{
	CSkeletalAnimationContainer* cont = instance.GetAnimatedComponent()->GetAnimationContainer();
	return cont->FindAnimationRestricted( animation );
}

Bool CBehaviorGraphAnimationMixerSlotNode::HasAnyCachedPose( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_cachedPosesNames ].Size() > 0;
}

Int32 CBehaviorGraphAnimationMixerSlotNode::FindCachedPose( CBehaviorGraphInstance& instance, const CName& animation ) const
{
	const TDynArray< CName >& cachedPosesNames = instance[ i_cachedPosesNames ];

	const Int32 size = cachedPosesNames.SizeInt();
	for ( Int32 i=0; i<size; ++i )
	{
		if ( cachedPosesNames[ i ] == animation )
		{
			return i;
		}
	}

	return -1;
}

Int32 CBehaviorGraphAnimationMixerSlotNode::AddCachedPose( CBehaviorGraphInstance& instance, const SAnimationFullState& animation ) const
{
	TDynArray< CName >& cachedPosesNames = instance[ i_cachedPosesNames ];
	TDynArray< CAllocatedBehaviorGraphOutput >& cachedPosesData = instance[ i_cachedPosesData ];

	if ( cachedPosesNames.Size() >= 32 || !animation.GetAnimation() )
	{
		return -1;
	}

	const Int32 index = (Int32)cachedPosesNames.Grow( 1 );
	cachedPosesData.Grow( 1 );

	ASSERT( cachedPosesData.Size() == cachedPosesNames.Size() );

	cachedPosesNames[ index ] = animation.m_state.m_animation;
	CAllocatedBehaviorGraphOutput& allocPose = cachedPosesData[ index ];

	allocPose.Create( instance, IsMimicsMode() );

	SBehaviorGraphOutput* pose = allocPose.GetPose();
	if ( pose )
	{
		animation.GetAnimation()->GetAnimation()->Sample( 0.f, pose->m_numBones, pose->m_numFloatTracks, pose->m_outputPose, pose->m_floatTracks );

		const CAnimatedComponent* ac = instance.GetAnimatedComponent();

		// Trajectory extraction
		if ( animation.m_extractTrajectory && ac->UseExtractedTrajectory() && ac->HasTrajectoryBone() )
		{
			pose->ExtractTrajectory( ac );
		}
		else if ( pose->m_numBones > 0 )
		{
#ifdef USE_HAVOK_ANIMATION
			pose->m_outputPose[ 0 ].m_rotation.normalize();
#else
			pose->m_outputPose[ 0 ].Rotation.Normalize();
#endif
		}
	}

	return index;
}

SBehaviorGraphOutput* CBehaviorGraphAnimationMixerSlotNode::GetCachedPose( CBehaviorGraphInstance& instance, Int32 index ) const
{
	TDynArray< CAllocatedBehaviorGraphOutput >& cachedPosesData = instance[ i_cachedPosesData ];

	ASSERT( cachedPosesData.SizeInt() > index );
	ASSERT( cachedPosesData[ index ].HasPose() );

	return cachedPosesData[ index ].GetPose();
}

Int32 CBehaviorGraphAnimationMixerSlotNode::FindMappedPose( CBehaviorGraphInstance& instance, Uint32 poseId ) const
{
	const TDynArray< Uint32 >& mappedPosesNames = instance[ i_mappedPosesIds ];

	const Int32 size = mappedPosesNames.SizeInt();
	for ( Int32 i=0; i<size; ++i )
	{
		if ( mappedPosesNames[ i ] == poseId )
		{
			return i;
		}
	}

	return -1;
}

Int32 CBehaviorGraphAnimationMixerSlotNode::FindMappedPoseByCorrection( CBehaviorGraphInstance& instance, CGUID correctionID ) const
{
	const TDynArray< SAnimationMappedPose >& mappedPosesData = instance[ i_mappedPosesData ];

	const Int32 size = mappedPosesData.SizeInt();
	for ( Int32 i=0; i<size; ++i )
	{
		if ( mappedPosesData[ i ].m_correctionID == correctionID )
		{
			return i;
		}
	}

	return -1;
}

Int32 CBehaviorGraphAnimationMixerSlotNode::AddMappedPose( CBehaviorGraphInstance& instance, Uint32 poseId, const SAnimationMappedPose& pose ) const
{
	TDynArray< Uint32 >& mappedPosesNames = instance[ i_mappedPosesIds ];
	TDynArray< SAnimationMappedPose >& mappedPosesData = instance[ i_mappedPosesData ];

	const Int32 index = (Int32)mappedPosesNames.Grow( 1 );
	mappedPosesData.Grow( 1 );

	ASSERT( mappedPosesData.Size() == mappedPosesNames.Size() );

	mappedPosesNames[ index ] = poseId;
	mappedPosesData[ index ] = pose;

	return index;
}

SAnimationMappedPose* CBehaviorGraphAnimationMixerSlotNode::GetMappedPose( CBehaviorGraphInstance& instance, Int32 index ) const
{
	TDynArray< SAnimationMappedPose >& mappedPosesData = instance[ i_mappedPosesData ];

	ASSERT( mappedPosesData.SizeInt() > index );

	return &(mappedPosesData[ index ]);
}

Bool CBehaviorGraphAnimationMixerSlotNode::HasAnimationWithID( CBehaviorGraphInstance& instance, const TInstanceVar< TDynArray< SAnimationFullState > >& data, const CGUID& id ) const
{
	ASSERT( !id.IsZero() );

	const TDynArray< SAnimationFullState >& arr = instance[ data ];
	const Uint32 num = arr.Size();
	for ( Uint32 i=0; i<num; ++i )
	{
		const SAnimationFullState& s = arr[ i ];
		if ( s.m_ID == id )
		{
			return true;
		}
	}
	
	return false;
}

Bool CBehaviorGraphAnimationMixerSlotNode::HasAnyAnimationWithID( CBehaviorGraphInstance& instance, const CGUID& id ) const
{
	return HasAnimationWithID( instance, i_animationsData, id ) || HasAnimationWithID( instance, i_additiveData, id ) || HasAnimationWithID( instance, i_overrideData, id );
}

Bool CBehaviorGraphAnimationMixerSlotNode::HasAnyIdleWithID( CBehaviorGraphInstance& instance, const CName& idleId ) const
{
	const SAnimationFullState& dataA = instance[ i_idleDataA ];
	const SAnimationFullState& dataB = instance[ i_idleDataB ];

	const Float weightAB = instance[ i_idleDataBlendWeight ];

	const Bool hasIdleA = dataA.m_state.m_animation == idleId && dataA.m_weight > 0.f && weightAB < 1.f;
	const Bool hasIdleB = dataB.m_state.m_animation == idleId && dataB.m_weight > 0.f && weightAB > 0.f;

	return hasIdleA || hasIdleB;
}

void CBehaviorGraphAnimationMixerSlotNode::RandAnimTime( SAnimationFullState& animation ) const
{
	const Float duration = animation.GetAnimation() ? animation.GetAnimation()->GetDuration() : 1.0f;
	const Float startTime = BehaviorUtils::RandF( duration );
	animation.m_state.m_prevTime = startTime;
	animation.m_state.m_currTime = startTime;
}

void CBehaviorGraphAnimationMixerSlotNode::OpenMixer( CBehaviorGraphInstance& instance ) const
{
	instance[ i_animationsData ].ClearFast();
	instance[ i_additiveData ].ClearFast();
	instance[ i_overrideData ].ClearFast();

	RemoveAllNotCorrectionPoses( instance );
}

void CBehaviorGraphAnimationMixerSlotNode::CloseMixer( CBehaviorGraphInstance& instance ) const
{
	RemoveAllUnusedCorrectionPoses( instance );
}

void CBehaviorGraphAnimationMixerSlotNode::AddAnimation( CBehaviorGraphInstance& instance, const TInstanceVar< TDynArray< SAnimationFullState > >& data, const SAnimationFullState& animation ) const
{
	TDynArray< SAnimationFullState >& arr = instance[ data ];

	arr.PushBack( animation );

	if ( !animation.GetAnimation() )
	{
		SAnimationFullState& newAnim = arr[ arr.Size() - 1 ];
		newAnim.SetAnimation( FindAnimation( instance, newAnim.m_state.m_animation ) );
	}
}

namespace
{
	Bool RemoveAnimationFrom( CBehaviorGraphInstance& instance, const TInstanceVar< TDynArray< SAnimationFullState > >& data, const CGUID& id )
	{
		TDynArray< SAnimationFullState >& arr = instance[ data ];

		const Uint32 size = arr.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const SAnimationFullState& a = arr[ i ];
			if ( a.m_ID == id )
			{
				arr.RemoveAtFast( i );
				return true;
			}
		}

		return false;
	}
}

void CBehaviorGraphAnimationMixerSlotNode::AddAnimationToSample( CBehaviorGraphInstance& instance, const SAnimationFullState& animation ) const
{
	AddAnimation( instance, i_animationsData, animation );
}

void CBehaviorGraphAnimationMixerSlotNode::AddAdditiveAnimationToSample( CBehaviorGraphInstance& instance, const SAnimationFullState& animation ) const
{
	AddAnimation( instance, i_additiveData, animation );
}

void CBehaviorGraphAnimationMixerSlotNode::AddOverrideAnimationToSample( CBehaviorGraphInstance& instance, const SAnimationFullState& animation ) const
{
	AddAnimation( instance, i_overrideData, animation );
}

Bool CBehaviorGraphAnimationMixerSlotNode::RemoveAnimation( CBehaviorGraphInstance& instance, const CGUID& id ) const
{
	Bool ret = false;
	ret |= RemoveAnimationFrom( instance, i_animationsData, id );
	ret |= RemoveAnimationFrom( instance, i_additiveData, id );
	ret |= RemoveAnimationFrom( instance, i_overrideData, id );
	return ret;
}

void CBehaviorGraphAnimationMixerSlotNode::RemoveAllAnimations( CBehaviorGraphInstance& instance ) const
{
	instance[ i_animationsData ].ClearFast();
	instance[ i_additiveData ].ClearFast();
	instance[ i_overrideData ].ClearFast();

	instance[ i_idleDataA ].Reset();
	instance[ i_idleDataB ].Reset();
	instance[ i_idleDataBlendWeight ] = 0.f;

	instance[ i_mappedPosesIds ].ClearFast();
	instance[ i_mappedPosesData ].ClearFast();

	instance[ i_cachedPosesNames ].Clear();

	TDynArray< CAllocatedBehaviorGraphOutput >& cachedPosesData = instance[ i_cachedPosesData ];
	const Uint32 size = cachedPosesData.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CAllocatedBehaviorGraphOutput& allocPose = cachedPosesData[ i ];

		if ( allocPose.HasPose() )
		{
			allocPose.Free( instance );
		}
	}
	cachedPosesData.Clear();

	ClearSyncTokens( instance );
}

namespace
{
	Bool AreIdlesVisuallyEqual( const SAnimationFullState& animA, const SAnimationFullState& animB )
	{
		return 
			animA.m_state.m_animation == animB.m_state.m_animation &&
			animA.m_weight == animB.m_weight;
	}
}

void CBehaviorGraphAnimationMixerSlotNode::SetIdleAnimationToSample( CBehaviorGraphInstance& instance, const SAnimationFullState& animationA, const SAnimationFullState& animationB, Float blendWeight, Bool canRandAnimStartTime, CAnimationMixerAnimSynchronizer* synchronizer ) const
{
	if ( !m_canUseIdles )
	{
		return;
	}

	SAnimationFullState& dataA = instance[ i_idleDataA ];
	SAnimationFullState& dataB = instance[ i_idleDataB ];

	Float& weight = instance[ i_idleDataBlendWeight ];

	SAnimationFullState prevAnimStateA = dataA;
	SAnimationFullState prevAnimStateB = dataB;

	dataA = animationA;
	dataB = animationB;

	const Bool resetA = !dataA.GetAnimation() && !AreIdlesVisuallyEqual( dataA, prevAnimStateA );
	const Bool resetB = !dataB.GetAnimation() && !AreIdlesVisuallyEqual( dataB, prevAnimStateB );

	if ( resetA )
	{
		dataA.SetAnimation( FindAnimation( instance, dataA.m_state.m_animation ) );
		if ( synchronizer && synchronizer->IsGetterMode() )
		{
			synchronizer->GetA( dataA.m_state.m_prevTime, dataA.m_state.m_currTime );
		}
		else if ( canRandAnimStartTime )
		{
			RandAnimTime( dataA );
		}
	}
	else
	{
		dataA = prevAnimStateA;
	}

	if ( resetB )
	{
		dataB.SetAnimation( FindAnimation( instance, dataB.m_state.m_animation ) );
		if ( synchronizer && synchronizer->IsGetterMode() )
		{
			synchronizer->GetB( dataB.m_state.m_prevTime, dataB.m_state.m_currTime );
		}
		else if ( canRandAnimStartTime )
		{
			RandAnimTime( dataB );
		}
	}
	else
	{
		dataB = prevAnimStateB;
	}

	const Bool contAWithB = AreIdlesVisuallyEqual( animationA, prevAnimStateB );
	if ( contAWithB )
	{
		dataA = prevAnimStateB;
	}

	weight = blendWeight;

	if ( synchronizer && synchronizer->IsSetterMode() )
	{
		synchronizer->Set( dataA.m_state.m_prevTime, dataA.m_state.m_currTime, dataB.m_state.m_prevTime, dataB.m_state.m_currTime );
	}
}

void CBehaviorGraphAnimationMixerSlotNode::ResetIdleAnimation( CBehaviorGraphInstance& instance ) const
{
	instance[ i_idleDataA ].Reset();
	instance[ i_idleDataB ].Reset();
}

void CBehaviorGraphAnimationMixerSlotNode::AddPoseToSample( CBehaviorGraphInstance& instance, Uint32 poseId, const SAnimationMappedPose& pose ) const
{
	TDynArray< SAnimationMappedPose >& mappedPosesData = instance[ i_mappedPosesData ];
	TDynArray< Uint32 >& mappedPosesNames = instance[ i_mappedPosesIds ];

	Int32 size		= mappedPosesData.SizeInt();
	Float weightSum = pose.m_weight;

	AddMappedPose( instance, poseId, pose );
	
	const Float MAX_WEIGHT = 1.f;
	for ( Int32 i = size - 1; i >=0; --i )
	{
		if ( mappedPosesData[ i ].m_correctionID == pose.m_correctionID || mappedPosesData[ i ].m_correctionIdleID == pose.m_correctionIdleID )
		{
			if ( weightSum >= MAX_WEIGHT || mappedPosesNames[ i ] == poseId )
			{
				mappedPosesNames.RemoveAt( i );
				mappedPosesData.RemoveAt( i );
			}
			else
			{
				Float weight = Min( mappedPosesData[i].m_weight,  MAX_WEIGHT - weightSum );
				mappedPosesData[i].m_weight = weight;
				weightSum += weight;
			}
		}
	}
}

void CBehaviorGraphAnimationMixerSlotNode::RemoveAllPoses( CBehaviorGraphInstance& instance ) const
{
	TDynArray< Uint32 >& mappedPosesNames = instance[ i_mappedPosesIds ];
	TDynArray< SAnimationMappedPose >& mappedPosesData = instance[ i_mappedPosesData ];

	mappedPosesNames.Clear();
	mappedPosesData.Clear();
}

void CBehaviorGraphAnimationMixerSlotNode::RemovePose( CBehaviorGraphInstance& instance, Uint32 poseId ) const
{
	const Int32 index = FindMappedPose( instance, poseId );
	ASSERT( index != -1 );

	if ( index != -1 )
	{
		TDynArray< Uint32 >& mappedPosesNames = instance[ i_mappedPosesIds ];
		TDynArray< SAnimationMappedPose >& mappedPosesData = instance[ i_mappedPosesData ];

		mappedPosesNames.RemoveAtFast( index );
		mappedPosesData.RemoveAtFast( index );
	}	
}

void CBehaviorGraphAnimationMixerSlotNode::RemoveAllNotCorrectionPoses( CBehaviorGraphInstance& instance ) const
{
	TDynArray< Uint32 >& mappedPosesNames = instance[ i_mappedPosesIds ];
	TDynArray< SAnimationMappedPose >& mappedPosesData = instance[ i_mappedPosesData ];

	const Int32 num = mappedPosesData.SizeInt();
	for ( Int32 i=num-1; i>=0; --i )
	{
		const SAnimationMappedPose& m = mappedPosesData[ i ];
		if ( m.m_correctionID.IsZero() && !m.m_correctionIdleID )
		{
			mappedPosesNames.RemoveAtFast( i );
			mappedPosesData.RemoveAtFast( i );
		}
	}
}

void CBehaviorGraphAnimationMixerSlotNode::RemoveAllUnusedCorrectionPoses( CBehaviorGraphInstance& instance ) const
{
	TDynArray< Uint32 >& mappedPosesNames = instance[ i_mappedPosesIds ];
	TDynArray< SAnimationMappedPose >& mappedPosesData = instance[ i_mappedPosesData ];

	const Int32 num = mappedPosesData.SizeInt();
	for ( Int32 i=num-1; i>=0; --i )
	{
		const SAnimationMappedPose& m = mappedPosesData[ i ];

		const Bool isEmptyKey = m.m_bones.Size() == 0 && m.m_tracks.Size() == 0;
		if ( isEmptyKey || ( (!m.m_correctionID.IsZero() && !HasAnyAnimationWithID( instance, m.m_correctionID )) || ( false /*m.m_correctionIdleID && !HasAnyIdleWithID( instance, m.m_correctionIdleID )*/) ) )
		{
			mappedPosesNames.RemoveAtFast( i );
			mappedPosesData.RemoveAtFast( i );
		}
	}
}

void CBehaviorGraphAnimationMixerSlotNode::GetState( const CBehaviorGraphInstance& instance, SAnimationMixerVisualState& state ) const
{
	const TDynArray< SAnimationFullState >& animationsData = instance[ i_animationsData ];
	const TDynArray< SAnimationFullState >& additiveData = instance[ i_additiveData ];
	const TDynArray< SAnimationFullState >& overrideData = instance[ i_overrideData ];

	state.m_animationsData.Reserve( animationsData.Size() );
	state.m_additiveData.Reserve( additiveData.Size() );
	state.m_overrideData.Reserve( overrideData.Size() );

	for ( const SAnimationFullState& s : animationsData )
	{
		new ( state.m_animationsData ) SAnimationMixerVisualState::AnimState( s );
	}
	for ( const SAnimationFullState& s : additiveData )
	{
		new ( state.m_additiveData ) SAnimationMixerVisualState::AnimState( s );
	}
	for ( const SAnimationFullState& s : overrideData )
	{
		new ( state.m_overrideData ) SAnimationMixerVisualState::AnimState( s );
	}


	const SAnimationFullState& idleA = instance[ i_idleDataA ];
	const SAnimationFullState& idleB = instance[ i_idleDataB ];

	state.m_idleA = SAnimationMixerVisualState::AnimState( idleA );
	state.m_idleB = SAnimationMixerVisualState::AnimState( idleB );
}

Bool CBehaviorGraphAnimationMixerSlotNode::ResamplePose( const CBehaviorGraphInstance& instance, SBehaviorGraphOutput& temp, Int32 boneIdx, Matrix& outBoneMS ) const
{
	Bool ret( true );

	const Bool bodyMode = IsBodyMode();
	ASSERT( bodyMode );

	Float bestAnimWeight = 0.f;
	Int32 bestAnimIdx = -1;
	const TDynArray< SAnimationFullState >& animDataArr = instance[ i_animationsData ];
	const Int32 size = animDataArr.SizeInt();
	for ( Int32 i=0; i<size; ++i )
	{
		const SAnimationFullState& animData = animDataArr[ i ];
		if ( animData.m_weight > bestAnimWeight )
		{
			bestAnimWeight = animData.m_weight;
			bestAnimIdx = i;
		}
	}

	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	if ( bestAnimIdx != -1 && bestAnimWeight > 0.5f )
	{
		const SAnimationFullState& animData = animDataArr[ bestAnimIdx ];
		if ( animData.GetAnimation() )
		{
			ret = animData.GetAnimation()->GetAnimation()->Sample( animData.m_state.m_currTime, temp.m_numBones, temp.m_numFloatTracks, temp.m_outputPose, temp.m_floatTracks );
		}
		else
		{
			ret = false;
		}
	}
	else
	{
		// Idle
		const SAnimationFullState& dataA = instance[ i_idleDataA ];
		const SAnimationFullState& dataB = instance[ i_idleDataB ];

		const Float wAB = instance[ i_idleDataBlendWeight ];
		ASSERT( wAB >= 0.f && wAB <= 1.f );

		if ( wAB < 0.5f ) // A
		{
			if ( dataA.GetAnimation() )
			{
				ret = dataA.GetAnimation()->GetAnimation()->Sample( dataA.m_state.m_currTime, temp.m_numBones, temp.m_numFloatTracks, temp.m_outputPose, temp.m_floatTracks );
			}
			else
			{
				ret = false;
			}
		}
		else // B
		{
			if ( dataB.GetAnimation() )
			{
				ret = dataB.GetAnimation()->GetAnimation()->Sample( dataB.m_state.m_currTime, temp.m_numBones, temp.m_numFloatTracks, temp.m_outputPose, temp.m_floatTracks );
			}
			else
			{
				ret = false;
			}
		}
	}

	if ( ret )
	{
		outBoneMS = temp.GetBoneModelMatrix( ac, boneIdx );
	}
	else
	{
		outBoneMS = Matrix::IDENTITY;
	}

	return ret;
}

#ifndef NO_EDITOR

const SAnimationFullState& CBehaviorGraphAnimationMixerSlotNode::GetIdleA( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_idleDataA ];
}

const SAnimationFullState& CBehaviorGraphAnimationMixerSlotNode::GetIdleB( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_idleDataB ];
}

const SAnimationFullState* CBehaviorGraphAnimationMixerSlotNode::GetAnimationState( CBehaviorGraphInstance& instance, CName animationName ) const
{
	const TDynArray< SAnimationFullState >& arr = instance[ i_animationsData ];
	for ( const SAnimationFullState& s : arr )
	{
		if ( s.m_state.m_animation == animationName )
		{
			return &s;
		}
	}
	return nullptr;
}

#endif

//////////////////////////////////////////////////////////////////////////

CBehaviorMixerSlotInterface::CBehaviorMixerSlotInterface()
	: m_slot( nullptr )
	, m_instance( nullptr )
	, m_instanceH( nullptr )
{

}

CBehaviorMixerSlotInterface::~CBehaviorMixerSlotInterface()
{

}

void CBehaviorMixerSlotInterface::Init( CBehaviorGraphAnimationMixerSlotNode* slot, CBehaviorGraphInstance* instance )
{
	m_slot = slot;
	m_instance = instance;
	m_instanceH = instance;
}

void CBehaviorMixerSlotInterface::Clear()
{
	m_slot = nullptr;
	m_instance = nullptr;
	m_instanceH = nullptr;
}

Bool CBehaviorMixerSlotInterface::IsValid() const
{
	return m_slot && m_instanceH.Get() && m_instance && m_instance->IsBinded();
}

CName CBehaviorMixerSlotInterface::GetInstanceName() const
{
	return IsValid() ? m_instance->GetInstanceName() : CName::NONE;
}

void CBehaviorMixerSlotInterface::OpenMixer()
{
	if ( IsValid() )
	{
		m_slot->OpenMixer( *m_instance );
	}
}

void CBehaviorMixerSlotInterface::CloseMixer()
{
	if ( IsValid() )
	{
		m_slot->CloseMixer( *m_instance );
	}
}

void CBehaviorMixerSlotInterface::AddAnimationToSample( const SAnimationFullState& animation )
{
	if ( IsValid() )
	{
		m_slot->AddAnimationToSample( *m_instance, animation );
	}
}
void CBehaviorMixerSlotInterface::SetIdleAnimationToSample( const SAnimationFullState& animationA, const SAnimationFullState& animationB, Float blendWeight, Bool canRandAnimStartTime, CAnimationMixerAnimSynchronizer* synchronizer )
{
	if ( IsValid() )
	{
		m_slot->SetIdleAnimationToSample( *m_instance, animationA, animationB, blendWeight, canRandAnimStartTime, synchronizer );
	}
}

void CBehaviorMixerSlotInterface::ResetIdleAnimation()
{
	if ( IsValid() )
	{
		m_slot->ResetIdleAnimation( *m_instance );
	}
}

void CBehaviorMixerSlotInterface::AddAdditiveAnimationToSample( const SAnimationFullState& animation )
{
	if ( IsValid() )
	{
		m_slot->AddAdditiveAnimationToSample( *m_instance, animation );
	}
}

void CBehaviorMixerSlotInterface::AddOverrideAnimationToSample( const SAnimationFullState& animation )
{
	if ( IsValid() )
	{
		m_slot->AddOverrideAnimationToSample( *m_instance, animation );
	}
}


void CBehaviorMixerSlotInterface::AddPoseToSample( Uint32 poseId, const SAnimationMappedPose& pose )
{
	if ( IsValid() )
	{
		m_slot->AddPoseToSample( *m_instance, poseId, pose );
	}
}

void CBehaviorMixerSlotInterface::RemovePose( Uint32 poseId )
{
	if ( IsValid() )
	{
		m_slot->RemovePose( *m_instance, poseId );
	}
}

void CBehaviorMixerSlotInterface::RemoveAllPoses()
{
	if ( IsValid() )
	{
		m_slot->RemoveAllPoses( *m_instance );
	}
}

void CBehaviorMixerSlotInterface::GetState( SAnimationMixerVisualState& state ) const
{
	if ( IsValid() )
	{
		m_slot->GetState( *m_instance, state );
	}
}

Bool CBehaviorMixerSlotInterface::ResamplePose( SBehaviorGraphOutput& tempA, Int32 boneIdx, Matrix& outBoneMS ) const
{
	return IsValid() ? m_slot->ResamplePose( *m_instance, tempA, boneIdx, outBoneMS ) : false;
}

Bool CBehaviorMixerSlotInterface::RemoveAnimation( const CGUID& id )
{
	return IsValid() ? m_slot->RemoveAnimation( *m_instance, id ) : false;
}

void CBehaviorMixerSlotInterface::RemoveAllAnimations()
{
	if ( IsValid() )
	{
		m_slot->RemoveAllAnimations( *m_instance );
	}
}

#ifndef NO_EDITOR

const SAnimationFullState* CBehaviorMixerSlotInterface::GetIdleA() const
{
	if ( IsValid() )
	{
		return &(m_slot->GetIdleA( *m_instance ));
	}
	else
	{
		return nullptr;
	}
}

const SAnimationFullState* CBehaviorMixerSlotInterface::GetIdleB() const
{
	if ( IsValid() )
	{
		return &(m_slot->GetIdleB( *m_instance ));
	}
	else
	{
		return nullptr;
	}
}

const SAnimationFullState* CBehaviorMixerSlotInterface::GetAnimationState( CName animationName ) const
{
	if ( IsValid() )
	{
		return m_slot->GetAnimationState( *m_instance, animationName );
	}
	else
	{
		return nullptr;
	}
}

#endif

//////////////////////////////////////////////////////////////////////////

namespace
{
	Bool Internal_IsNotEqual( const TDynArray< SAnimationMixerVisualState::AnimState >& animsA, const TDynArray< SAnimationMixerVisualState::AnimState >& animsB )
	{
		Bool ret( false );

		for ( const SAnimationMixerVisualState::AnimState& sA : animsA )
		{
			if ( sA.m_weight < 0.01f )
			{
				continue;
			}

			Bool found( false );

			for ( const SAnimationMixerVisualState::AnimState& sB : animsB )
			{
				if ( sA.IsEqual( sB ) )
				{
					found = true;
					break;
				}
			}

			if ( !found )
			{
				ret = true;
				break;
			}
		}

		return ret;
	}
}

Bool SAnimationMixerVisualState::IsNotEqual( const SAnimationMixerVisualState& stateA, const SAnimationMixerVisualState& stateB )
{
	Bool ret( false );

	if (	!stateA.m_idleA.IsEqual( stateB.m_idleA ) ||
			!stateA.m_idleB.IsEqual( stateB.m_idleB ) )
	{
		ret = true;
	}

	if ( !ret )
	{
		ret = Internal_IsNotEqual( stateA.m_animationsData, stateB.m_animationsData );

		if ( !ret )
		{
			ret = Internal_IsNotEqual( stateB.m_animationsData, stateA.m_animationsData );

			if ( !ret )
			{
				ret = Internal_IsNotEqual( stateA.m_additiveData, stateB.m_additiveData );

				if ( !ret )
				{
					ret = Internal_IsNotEqual( stateB.m_additiveData, stateA.m_additiveData );

					if ( !ret )
					{
						ret = Internal_IsNotEqual( stateA.m_overrideData, stateB.m_overrideData );

						if ( !ret )
						{
							ret = Internal_IsNotEqual( stateB.m_overrideData, stateA.m_overrideData );
						}
					}
				}
			}
		}
	}
	
	return ret;
}

//////////////////////////////////////////////////////////////////////////

CAnimationMixerAnimSynchronizer::CAnimationMixerAnimSynchronizer()
	: m_isSetterMode( false )
	, m_currTimeA( 0.f )
	, m_prevTimeA( 0.f )
	, m_currTimeB( 0.f )
	, m_prevTimeB( 0.f )
#ifdef RED_ASSERTS_ENABLED
	, m_initialized( false )
#endif
{

}

void CAnimationMixerAnimSynchronizer::SetGetterMode()
{
#ifdef RED_ASSERTS_ENABLED
	m_initialized = true;
#endif
	m_isSetterMode = false;
}

void CAnimationMixerAnimSynchronizer::SetSetterMode()
{
#ifdef RED_ASSERTS_ENABLED
	m_initialized = true;
#endif
	m_isSetterMode = true;
}

Bool CAnimationMixerAnimSynchronizer::IsGetterMode() const
{
#ifdef RED_ASSERTS_ENABLED
	RED_ASSERT( m_initialized );
#endif
	return !m_isSetterMode;
}

Bool CAnimationMixerAnimSynchronizer::IsSetterMode() const
{
#ifdef RED_ASSERTS_ENABLED
	RED_ASSERT( m_initialized );
#endif
	return m_isSetterMode;
}

void CAnimationMixerAnimSynchronizer::Set( Float prevTimeA, Float currTimeA, Float prevTimeB, Float currTimeB )
{
#ifdef RED_ASSERTS_ENABLED
	RED_ASSERT( m_initialized );
#endif
	m_prevTimeA = prevTimeA;
	m_currTimeA = currTimeA;
	m_prevTimeB = prevTimeB;
	m_currTimeB = currTimeB;
}

void CAnimationMixerAnimSynchronizer::GetA( Float& prevTime, Float& currTime ) const
{
#ifdef RED_ASSERTS_ENABLED
	RED_ASSERT( m_initialized );
#endif
	prevTime = m_prevTimeA;
	currTime = m_currTimeA;
}

void CAnimationMixerAnimSynchronizer::GetB( Float& prevTime, Float& currTime ) const
{
#ifdef RED_ASSERTS_ENABLED
	RED_ASSERT( m_initialized );
#endif
	prevTime = m_prevTimeB;
	currTime = m_currTimeB;
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_MIXER
#pragma optimize("",on)
#endif
