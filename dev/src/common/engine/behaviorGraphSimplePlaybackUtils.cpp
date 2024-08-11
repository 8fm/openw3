/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "behaviorGraphSimplePlaybackUtils.h"
#include "behaviorGraphBlendNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphContext.h"
#include "cacheBehaviorGraphOutput.h"
#include "behaviorGraphUtils.inl"
#include "skeletalAnimationEntry.h"
#include "animatedComponent.h"

//////////////////////////////////////////////////////////////////////////

// #define DEBUG_SIMPLE_PLAYBACK

#ifdef DEBUG_SIMPLE_PLAYBACK
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SSimpleAnimationPlayback );
IMPLEMENT_ENGINE_CLASS( SSimpleAnimationPlaybackSet );

//////////////////////////////////////////////////////////////////////////

void SSimpleAnimationPlayback::ReadyForAnimSwitch( Float modifyUsageWeightSubtract )
{
	m_autoUpdateRequired = true;
	m_prevUsageWeight = Max( 0.0f, m_usageWeight - modifyUsageWeightSubtract );
	if (m_prevUsageWeight < 0.0f)
	{
		m_prevUsageWeight = 0.0f;
	}
	m_usageWeight = 0.0f;
}

void SSimpleAnimationPlayback::SwitchToAnim( const CSkeletalAnimationSetEntry* animation, Float weight )
{
	if ( m_animation != animation )
	{
		if ( m_animation && m_animation->GetAnimation() )
		{
			m_animation->GetAnimation()->ReleaseUsage();
		}
		m_animation = animation;
		if ( m_animation && m_animation->GetAnimation() )
		{
			m_animation->GetAnimation()->AddUsage();
		}
		if ( m_animation )
		{
			m_prevUsageWeight = weight;
			m_usageWeight = weight;
			m_blendDuration = BehaviorUtils::GetTimeFromCompressedBlend( m_animation->GetCompressedPoseBlend() );
			m_blendTimeLeft = m_animation->GetAnimation()->IsLoaded()? 0.0f : m_blendDuration;
		}
		else
		{
			m_prevUsageWeight = 0.0f;
			m_usageWeight = 0.0f;
		}
	}
	else
	{
		m_usageWeight += weight;
	}
}

void SSimpleAnimationPlayback::AddUsageWeight( Float weight )
{
	m_usageWeight += weight;
}

void SSimpleAnimationPlayback::SwitchOffAnimIfNotUsedAndReadyForSampling( Float blendTime, Float timeDelta, Bool anyActive )
{
	if ( m_usageWeight == 0.0f && ! anyActive )
	{
		// allow to slowly decay, if it reaches 0 (check ReadyForAnimSwitch), we will switch it off here
		// but this should happen only if there is no active anim
		m_usageWeight = m_prevUsageWeight;
	}
	else
	{
		// blend with previous
		m_usageWeight = blendTime > 0.0f? BlendOnOffWithSpeedBasedOnTime( m_prevUsageWeight, m_usageWeight, blendTime, timeDelta ) : m_usageWeight;
	}
	if ( m_usageWeight == 0.0f)
	{
		SwitchToAnim( NULL );
	}
	// ready for next frame
	m_prevUsageWeight = 0.0f;
	m_samplingWeight = m_usageWeight;
}

void SSimpleAnimationPlayback::SetSamplingWeightAsUsageWeight()
{
	m_samplingWeight = m_usageWeight;
}

void SSimpleAnimationPlayback::SetTime( Float currTime, Float prevTime, Int32 loops, Float playbackSpeed, Bool looped)
{
	m_currTime = currTime;
	m_prevTime = prevTime >= 0.0f? prevTime : m_currTime;
	m_loops = loops;
	m_playbackSpeed = 0.0f;
	m_looped = false;
	m_autoUpdateRequired = false;
}

void SSimpleAnimationPlayback::SetupPlayback( Float currTime, Float playbackSpeed, Bool looped )
{
	m_playbackSpeed = playbackSpeed;
	m_looped = looped;
	m_currTime = currTime;
	m_prevTime = currTime;
	m_loops = 0;
	m_autoUpdateRequired = false;
}

void SSimpleAnimationPlayback::UpdatePlayback( Float timeDelta )
{
	m_loops = 0;
	m_prevTime = m_currTime;
	m_currTime += m_playbackSpeed * timeDelta;
	Float duration = m_animation->GetDuration();
	if ( m_looped )
	{
		while ( m_currTime >= duration )
		{
			m_currTime -= duration;
			++ m_loops;
		}
		while ( m_currTime < 0.0f )
		{
			m_currTime += duration;
			-- m_loops;
		}
	}
	else
	{
		m_currTime = Clamp( m_currTime, 0.0f, duration );
	}
}

void SSimpleAnimationPlayback::SamplePlayback( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float timeDelta, Bool getMotion )
{
	if ( m_autoUpdateRequired )
	{
		UpdatePlayback( timeDelta );
		m_autoUpdateRequired = false;
	}
	Sample( context, instance, output, timeDelta, m_currTime, m_prevTime, m_loops, getMotion );
}

void SSimpleAnimationPlayback::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float timeDelta, Float currTime, Float prevTime, Int32 loops, Bool getMotion )
{
	m_currTime = currTime;
	m_prevTime = prevTime;

	const CAnimatedComponent* animatedComponent = instance.GetAnimatedComponent();

	const CSkeletalAnimation* skAnimation = m_animation ? m_animation->GetAnimation() : NULL;

	if ( skAnimation )
	{
		Bool ret = false;
#ifdef DISABLE_SAMPLING_AT_LOD3
		if ( context.GetLodLevel() <= BL_Lod2 )
#endif
		{
			const CSkeleton* skeleton = animatedComponent->GetSkeleton();
			ret = UpdateAndSampleBlendWithCompressedPose( m_animation.Get(), timeDelta, m_blendTimeLeft, currTime, context, output, skeleton );
		}

		if ( ret )
		{
			output.Touch();
		}

		// Pose correction
		if ( context.ShouldCorrectPose() )
		{
			context.SetPoseCorrection( output );
		}

		// Trajectory extraction
		if ( animatedComponent->UseExtractedTrajectory() && animatedComponent->HasTrajectoryBone() )
		{
			output.ExtractTrajectory( animatedComponent );
		}
		else if ( output.m_numBones > 0 )
		{
#ifdef USE_HAVOK_ANIMATION // VALID
			output.m_outputPose[ 0 ].m_rotation.normalize();
#else
			output.m_outputPose[ 0 ].Rotation.Normalize();
#endif
		}

		if ( getMotion && skAnimation->HasExtractedMotion() )
		{
			output.m_deltaReferenceFrameLocal = skAnimation->GetMovementBetweenTime( prevTime, currTime, loops );
		}
		else
		{
			// no delta here
			output.m_deltaReferenceFrameLocal = AnimQsTransform::IDENTITY;
		}

		Float alpha = 1.0f;
		m_animation->GetEventsByTime( prevTime, currTime, loops, alpha, NULL, &output );

		// append used anim
		output.AppendUsedAnim( SBehaviorUsedAnimationData( m_animation.Get(), currTime, 1.0f, 0.0f, false ) );
	}
}

void SSimpleAnimationPlayback::GetSyncInfo( CSyncInfo &info ) const
{
	info.m_currTime = m_currTime;
	info.m_prevTime = m_prevTime;
	info.m_totalTime = m_animation.Get() && m_animation->GetAnimation() ? m_animation->GetAnimation()->GetDuration() : 0.0f;
	info.m_wantSyncEvents = false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void SSimpleAnimationPlayback::CollectAnimationUsageData( const CBehaviorGraphInstance& instance, TDynArray<SBehaviorUsedAnimationData>& collectorArray, Float weight ) const
{
	SBehaviorUsedAnimationData usageInfo( m_animation.Get(), m_currTime, m_samplingWeight * weight );

	collectorArray.PushBack( usageInfo );
}
#endif

//////////////////////////////////////////////////////////////////////////

SSimpleAnimationPlaybackSet::SSimpleAnimationPlaybackSet()
	:	m_storedTimeDelta( 0.0f )
{
	m_playbacks.Resize(32);
}

void SSimpleAnimationPlaybackSet::ClearAnims()
{
	for ( TDynArray< SSimpleAnimationPlayback >::iterator iPb = m_playbacks.Begin(); iPb != m_playbacks.End(); ++ iPb )
	{
		iPb->SwitchToAnim( NULL );
	}
}

void SSimpleAnimationPlaybackSet::ReadyForAnimSwitch( Float modifyUsageWeightSubtract )
{
	for ( TDynArray< SSimpleAnimationPlayback >::iterator iPb = m_playbacks.Begin(); iPb != m_playbacks.End(); ++ iPb )
	{
		iPb->ReadyForAnimSwitch( modifyUsageWeightSubtract );
	}
}

SSimpleAnimationPlayback* SSimpleAnimationPlaybackSet::AddAnim( const CSkeletalAnimationSetEntry* animation, Float weight )
{
	if ( ! animation || weight <= 0.0f )
	{
		// nothing to add here
		return NULL;
	}
	// go through all and find matching anim
	for ( TDynArray< SSimpleAnimationPlayback >::iterator iPb = m_playbacks.Begin(); iPb != m_playbacks.End(); ++ iPb )
	{
		if ( iPb->GetAnimation() == animation )
		{
			iPb->AddUsageWeight( weight );
			return &(*iPb);
		}
	}
	// go through all and find not used slot
	for ( TDynArray< SSimpleAnimationPlayback >::iterator iPb = m_playbacks.Begin(); iPb != m_playbacks.End(); ++ iPb )
	{
		if ( ! iPb->IsUsed() )
		{
			iPb->SwitchToAnim( animation, weight );
			return &(*iPb);
		}
	}
	ASSERT( false, TXT( "We couldn't find empty slot for animation to add. It will break only the looks" ) );
	return NULL;
}

inline Float TimeDistance( Float timeA, Float timeB, Bool looped, Float duration )
{
	Float distance = Abs( timeA - timeB );
	if ( looped )
	{
		if ( distance > 0.5f * duration )
		{
			distance = Abs( distance - duration );
		}
	}
	return distance;
}

SSimpleAnimationPlayback* SSimpleAnimationPlaybackSet::AddAnimAt( const CSkeletalAnimationSetEntry* animation, Float currTime, Float prevTime, Int32 loops, Float playbackSpeed, Bool looped, Float weight )
{
	if ( ! animation || weight <= 0.0f )
	{
		// nothing to add here
		return NULL;
	}
	// go through all and find matching anim
	for ( TDynArray< SSimpleAnimationPlayback >::iterator iPb = m_playbacks.Begin(); iPb != m_playbacks.End(); ++ iPb )
	{
		if ( iPb->GetAnimation() == animation && 
			 ( TimeDistance( currTime, iPb->GetCurrTime(), looped, animation->GetDuration() ) < 0.1f ||
			   TimeDistance( prevTime, iPb->GetCurrTime(), looped, animation->GetDuration() ) < 0.1f ) ) // we're at the same point in time or we just were at that point in time
		{
			iPb->AddUsageWeight( weight );
			iPb->SetTime( currTime, prevTime, loops, playbackSpeed, looped );
			return &(*iPb);
		}
#ifdef DEBUG_SIMPLE_PLAYBACK
		else if ( iPb->GetAnimation() == animation )
		{
			RED_LOG(syncAnims, TXT("we have two anims with same name %s one at %.3f (prev %.3f) and existing at %.3f - we have to create new one!!!"), animation->GetName().AsString().AsChar(), currTime, prevTime, iPb->GetCurrTime() );
		}
#endif
	}
	// go through all and find not used slot
	for ( TDynArray< SSimpleAnimationPlayback >::iterator iPb = m_playbacks.Begin(); iPb != m_playbacks.End(); ++ iPb )
	{
		if ( ! iPb->IsUsed() )
		{
			iPb->SwitchToAnim( animation, weight );
			iPb->SetTime( currTime, prevTime, loops, playbackSpeed, looped );
			return &(*iPb);
		}
	}
	ASSERT( false, TXT( "We couldn't find empty slot for animation to add. It will break only the looks" ) );
	return NULL;
}

void SSimpleAnimationPlaybackSet::SwitchOffAnimIfNotUsedAndReadyForSampling()
{
	Bool anyActive;
	{
		Float usageWeight = 0.0f;
		for ( TDynArray< SSimpleAnimationPlayback >::iterator iPb = m_playbacks.Begin(); iPb != m_playbacks.End(); ++ iPb )
		{
			if ( iPb->IsUsed() )
			{
				usageWeight += iPb->GetUsageWeight();
			}
		}
		anyActive = usageWeight > 0.0f;
	}
	for ( TDynArray< SSimpleAnimationPlayback >::iterator iPb = m_playbacks.Begin(); iPb != m_playbacks.End(); ++ iPb )
	{
		iPb->SwitchOffAnimIfNotUsedAndReadyForSampling( 0.0f, 0.0f, anyActive );
	}
}

Float SSimpleAnimationPlaybackSet::SwitchOffAnimIfNotUsedAndReadyForSamplingWithNormalization( Float blendTime, Float timeDelta )
{
	Bool anyActive;
	{
		Float usageWeight = 0.0f;
		for ( TDynArray< SSimpleAnimationPlayback >::iterator iPb = m_playbacks.Begin(); iPb != m_playbacks.End(); ++ iPb )
		{
			if ( iPb->IsUsed() )
			{
				usageWeight += iPb->GetUsageWeight();
			}
		}
		anyActive = usageWeight > 0.0f;
	}
	Float weight = 0.0f;
	for ( TDynArray< SSimpleAnimationPlayback >::iterator iPb = m_playbacks.Begin(); iPb != m_playbacks.End(); ++ iPb )
	{
		if ( iPb->IsUsed() )
		{
			iPb->SwitchOffAnimIfNotUsedAndReadyForSampling( blendTime, timeDelta, anyActive );
			weight += iPb->GetUsageWeight();
		}
	}
	if ( weight != 1.0f && weight > 0.0f )
	{
		Float invWeight = 1.0f / weight;
		for ( TDynArray< SSimpleAnimationPlayback >::iterator iPb = m_playbacks.Begin(); iPb != m_playbacks.End(); ++ iPb )
		{
			if ( iPb->IsUsed() )
			{
				iPb->MultiplySamplingWeightBy( invWeight );
			}
		}
	}
	return weight;
}

void SSimpleAnimationPlaybackSet::SetupPlayback( const SBehaviorUsedAnimations& usedAnimations )
{
	ClearAnims();

#ifdef DEBUG_SIMPLE_PLAYBACK
	Int32 idx = 0;
	RED_LOG(syncAnims, TXT("setup playback") );
#endif
	// use only normal anims
	const SBehaviorUsedAnimationData * usedAnim = usedAnimations.m_anims.GetUsedData();
	for ( Uint32 idx = 0; idx < usedAnimations.m_anims.GetNum(); ++ idx, ++ usedAnim )
	{
		if ( usedAnim->m_animation )
		{
#ifdef DEBUG_SIMPLE_PLAYBACK
			Int32 idx = 0;
			RED_LOG(syncAnims, TXT(" add %s %.3f"), usedAnim->m_animation? usedAnim->m_animation->GetName().AsChar() : TXT("--"), usedAnim->m_weight);
#endif
			if ( SSimpleAnimationPlayback* addedPlayback = AddAnim( usedAnim->m_animation, usedAnim->m_weight ) )
			{
				addedPlayback->SetupPlayback( usedAnim->m_currTime, usedAnim->m_playbackSpeed, usedAnim->m_looped );
				addedPlayback->SetSamplingWeightAsUsageWeight();
			}
		}
	}
}

void SSimpleAnimationPlaybackSet::UpdatePlayback( Float timeDelta )
{
	m_storedTimeDelta = timeDelta;

	for ( TDynArray< SSimpleAnimationPlayback >::iterator iPb = m_playbacks.Begin(); iPb != m_playbacks.End(); ++ iPb )
	{
		if ( iPb->IsUsed() )
		{
			iPb->UpdatePlayback( timeDelta );
		}
	}
}

void SSimpleAnimationPlaybackSet::SamplePlaybackWeighted( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Bool getMotion )
{
	CCacheBehaviorGraphOutput cachePosePlayback( context );
	SBehaviorGraphOutput* playbackPose = cachePosePlayback.GetPose();

	Float weightSoFar = 0.0f;
	for ( TDynArray< SSimpleAnimationPlayback >::iterator iPb = m_playbacks.Begin(); iPb != m_playbacks.End(); ++ iPb )
	{
		if ( iPb->IsUsed() )
		{
			const Float weight = iPb->GetSamplingWeight();
			if ( weight > 0.0f )
			{
				if ( weightSoFar == 0.0f )
				{
					iPb->SamplePlayback( context, instance, output, m_storedTimeDelta, getMotion );
					weightSoFar = weight;
				}
				else
				{
					const Float weightTotalSoFar = weightSoFar + weight;
					const Float outputInterpolationWeight = weightSoFar / weightTotalSoFar;
					const Float newInterpolationWeight = weight / weightTotalSoFar;
					playbackPose->Reset();
					iPb->SamplePlayback( context, instance, *playbackPose, m_storedTimeDelta, getMotion );
					output.SetInterpolate( output, *playbackPose, newInterpolationWeight );
					output.MergeUsedAnims( output, *playbackPose, outputInterpolationWeight, newInterpolationWeight );
					weightSoFar = weightTotalSoFar;
				}
			}
		}
	}
}

void SSimpleAnimationPlaybackSet::SampleAdditively( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float timeDelta, Float currTime, Float prevTime, Int32 loops, Bool getMotion )
{
	CCacheBehaviorGraphOutput cachePosePlayback( context );
	SBehaviorGraphOutput* playbackPose = cachePosePlayback.GetPose();

	for ( TDynArray< SSimpleAnimationPlayback >::iterator iPb = m_playbacks.Begin(); iPb != m_playbacks.End(); ++ iPb )
	{
		if ( iPb->IsUsed() )
		{
			playbackPose->Reset();
			iPb->Sample( context, instance, *playbackPose, timeDelta, currTime, prevTime, loops, getMotion );
			output.SetAddMul( *playbackPose, iPb->GetSamplingWeight() );
			output.MergeUsedAnims( *playbackPose, iPb->GetSamplingWeight() );
		}
	}
}

void SSimpleAnimationPlaybackSet::SamplePlayback( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float timeDelta, Bool getMotion )
{
	CCacheBehaviorGraphOutput cachePosePlayback( context );
	SBehaviorGraphOutput* playbackPose = cachePosePlayback.GetPose();

#ifdef DEBUG_SIMPLE_PLAYBACK
	Int32 idx = 0;
	RED_LOG(syncAnims, TXT("sample playback") );
#endif

	Float totalSumOfWeights = 0.0f;
	for ( TDynArray< SSimpleAnimationPlayback >::iterator iPb = m_playbacks.Begin(); iPb != m_playbacks.End(); ++ iPb )
	{
		if ( iPb->IsUsed() )
		{
			totalSumOfWeights += iPb->GetSamplingWeight();
		}
	}

	Float weightSoFar = 0.0f;
	for ( TDynArray< SSimpleAnimationPlayback >::iterator iPb = m_playbacks.Begin(); iPb != m_playbacks.End(); ++ iPb )
	{
		if ( iPb->IsUsed() )
		{
			playbackPose->Reset();
			iPb->SamplePlayback( context, instance, *playbackPose, timeDelta, getMotion );
			weightSoFar += iPb->GetSamplingWeight();

			// This may look suspicious, cuz interpolation is done with different weight than merging events and anims.
			// This is because of strictly technical reason - we are using linear blending between poses here, 
			// so actually each anim is combined with (weight_of_anim/sum_of_weights) portion.
			output.SetInterpolate( output, *playbackPose, iPb->GetSamplingWeight() / weightSoFar );
			output.MergeEventsAndUsedAnims( *playbackPose, iPb->GetSamplingWeight() / totalSumOfWeights );

#ifdef DEBUG_SIMPLE_PLAYBACK
			RED_LOG(syncAnims, TXT("sample %i %.3f (%.3f) @%.3f %s"), idx, iPb->GetSamplingWeight(), iPb->GetUsageWeight(), iPb->GetCurrTime(), iPb->GetAnimation()->GetName().AsChar() );
#endif
		}
#ifdef DEBUG_SIMPLE_PLAYBACK
		++ idx;
#endif
	}
}

void SSimpleAnimationPlaybackSet::GetSyncInfo( CSyncInfo &info ) const
{
	const SSimpleAnimationPlayback* bestPlayback = NULL;
	Float topWeight = 0.0f;
	for ( TDynArray< SSimpleAnimationPlayback >::const_iterator iPb = m_playbacks.Begin(); iPb != m_playbacks.End(); ++ iPb )
	{
		if ( iPb->IsUsed() )
		{
			const Float weight = iPb->GetSamplingWeight();
			if ( topWeight < weight )
			{
				topWeight = weight;
				bestPlayback = &(*iPb);
			}
		}
	}

	if ( bestPlayback )
	{
		bestPlayback->GetSyncInfo( info );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void SSimpleAnimationPlaybackSet::CollectAnimationUsageData( const CBehaviorGraphInstance& instance, TDynArray<SBehaviorUsedAnimationData>& collectorArray, Float weight ) const
{
	for ( TDynArray< SSimpleAnimationPlayback >::const_iterator iPb = m_playbacks.Begin(); iPb != m_playbacks.End(); ++ iPb )
	{
		if ( iPb->IsUsed() )
		{
			iPb->CollectAnimationUsageData( instance, collectorArray, weight );
		}
	}
}
#endif

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_SIMPLE_PLAYBACK
#pragma optimize("",on)
#endif
