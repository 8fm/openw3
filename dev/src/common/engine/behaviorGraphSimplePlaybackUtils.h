/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

///////////////////////////////////////////////////////////////////////////////

class CBehaviorGraphValueNode;

///////////////////////////////////////////////////////////////////////////////

#include "behaviorGraphNode.h"
#include "skeletalAnimationEntry.h"

///////////////////////////////////////////////////////////////////////////////

struct SBehaviorUsedAnimations;

//
//	Animation playback util
//

struct SSimpleAnimationPlayback
{
	DECLARE_RTTI_STRUCT( SSimpleAnimationPlayback );

private:
	THandle<CSkeletalAnimationSetEntry> m_animation;
	Float m_blendDuration;
	Float m_blendTimeLeft;
	Float m_prevUsageWeight;
	Float m_usageWeight;
	Float m_samplingWeight;
	Bool m_autoUpdateRequired;

	// doesn't have to be used
	Float m_playbackSpeed;
	Bool m_looped;

	Float m_currTime;
	Float m_prevTime;
	Int32 m_loops;

public:
	SSimpleAnimationPlayback()
		:	m_animation( NULL )
		,	m_blendDuration( 0.1f )
		,	m_blendTimeLeft( 0.0f )
		,	m_prevUsageWeight( 0.0f )
		,	m_usageWeight( 0.0f )
		,	m_samplingWeight( 0.0f )
		,	m_playbackSpeed( 1.0f )
		,	m_looped( false )
		,	m_currTime( 0.0f )
		,	m_prevTime( 0.0f )
		,	m_loops( 0 )
	{}

	RED_INLINE const CSkeletalAnimationSetEntry* GetAnimation() const { return m_animation.Get(); }
	RED_INLINE Bool IsUsed() const { return m_animation.Get() != NULL; } // checks animation instead of weight, as during anim switch it is safer
	RED_INLINE Float GetUsageWeight() const { return m_usageWeight; }
	RED_INLINE Float GetSamplingWeight() const { return m_samplingWeight; }
	RED_INLINE void MultiplySamplingWeightBy( Float _by ) { m_samplingWeight *= _by; }
	RED_INLINE Float GetCurrTime() const { return m_currTime; }
	RED_INLINE Float GetPrevTime() const { return m_prevTime; }

	RED_INLINE void ReadyForAnimSwitch( Float modifyUsageWeightSubtract = 0.0f );
	RED_INLINE void SwitchToAnim( const CSkeletalAnimationSetEntry* animation, Float weight = 1.0f );
	RED_INLINE void AddUsageWeight( Float weight );
	RED_INLINE void SwitchOffAnimIfNotUsedAndReadyForSampling( Float blendTime = 0.0f, Float timeDelta = 0.0f, Bool anyActive = false );
	RED_INLINE void SetSamplingWeightAsUsageWeight();

	/** playback methods */
	RED_INLINE void SetupPlayback( Float currTime, Float playbackSpeed, Bool looped );
	RED_INLINE void UpdatePlayback( Float timeDelta );
	RED_INLINE void SamplePlayback( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float timeDelta, Bool getMotion = false );
	RED_INLINE void SetTime( Float currTime, Float prevTime = -1.0f, Int32 loops = 0, Float playbackSpeed = 0.0f, Bool looped = true );

	/** just sampling */
	RED_INLINE void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float timeDelta, Float currTime, Float prevTime = -1.0f, Int32 loops = 0, Bool getMotion = false );

	RED_INLINE void GetSyncInfo( CSyncInfo &info ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual void CollectAnimationUsageData( const CBehaviorGraphInstance& instance, TDynArray<SBehaviorUsedAnimationData>& collectorArray, Float weight = 1.0f ) const;
#endif

protected:
};

BEGIN_CLASS_RTTI( SSimpleAnimationPlayback );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
//
//	Animation playback set
//

struct SSimpleAnimationPlaybackSet
{
	DECLARE_RTTI_STRUCT( SSimpleAnimationPlaybackSet );

	TDynArray< SSimpleAnimationPlayback > m_playbacks;
	Float m_storedTimeDelta; // from last update

	SSimpleAnimationPlaybackSet();

	void ClearAnims();
	void ReadyForAnimSwitch( Float modifyUsageWeightSubtract = 0.0f );
	SSimpleAnimationPlayback* AddAnim( const CSkeletalAnimationSetEntry* animation, Float weight = 1.0f );
	SSimpleAnimationPlayback* AddAnimAt( const CSkeletalAnimationSetEntry* animation, Float currTime, Float prevTime, Int32 loops, Float playbackSpeed, Bool looped, Float weight = 1.0f );
	void SwitchOffAnimIfNotUsedAndReadyForSampling();
	Float SwitchOffAnimIfNotUsedAndReadyForSamplingWithNormalization( Float blendTime = 0.0f, Float timeDelta = 0.0f ); // returns non normalized weight

	/** playback methods */
	void SetupPlayback( const SBehaviorUsedAnimations& usedAniamtions );
	void UpdatePlayback( Float timeDelta );
	void SamplePlaybackWeighted( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Bool getMotion = false );

	/** Just sample */
	void SamplePlayback( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float timeDelta, Bool getMotion = false );
	void SampleAdditively( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float timeDelta, Float currTime, Float prevTime = -1.0f, Int32 loops = 0, Bool getMotion = false );

	void GetSyncInfo( CSyncInfo &info ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual void CollectAnimationUsageData( const CBehaviorGraphInstance& instance, TDynArray<SBehaviorUsedAnimationData>& collectorArray, Float weight = 1.0f ) const;
#endif
};

BEGIN_CLASS_RTTI( SSimpleAnimationPlaybackSet );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
