/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "playedAnimation.h"
#include "skeletalAnimation.h"

class IAnimatedSkeletonConstraint;

/// Animated instance of skeleton
class CAnimatedSkeleton
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Animation );

protected:
	const CSkeleton*						m_baseSkeleton;			//!< Source skeleton
	TDynArray< CPlayedSkeletalAnimation*, MC_Animation >	m_animations;			//!< Playing animations

	Uint32														m_constraintLastId;
	TDynArray< TPair< Uint32, IAnimatedSkeletonConstraint* >, MC_Animation >	m_constraints;

public:
	CAnimatedSkeleton( const CSkeleton *skeleton );
	~CAnimatedSkeleton();

public:
	//! Change skeleton in runtime
	void SetSkeleton( const CSkeleton *skeleton );

	// Get skeleton
	const CSkeleton* GetSkeleton() const { return m_baseSkeleton; }

	//! Get played animation
	CPlayedSkeletalAnimation* GetPlayedAnimation( Uint32 num );

	//! Get played animation
	CPlayedSkeletalAnimation* GetPlayedAnimation( const CName& name );

	//////////////////////////////////////////////////////////////////////////

public:
	//! Advance animations
	void Update( CAnimatedComponent* ac, Float deltaT );

	//! Sample
	SBehaviorGraphOutput& Sample( CAnimatedComponent* ac, SBehaviorSampleContext* context ) const;

	//! Play animation on this skeleton
	CPlayedSkeletalAnimation* PlayAnimation( CSkeletalAnimationSetEntry *animation, 
											 Bool repleace = true, Bool looped = true, Float weight = 1.f, 
											 ESkeletalAnimationType type = SAT_Normal,
											 Bool autoFireEffects = true );

	//! Stop all animation
	void StopAllAnimation();

	//! Is playing any animation
	Bool IsPlayingAnyAnimation() const;

	//! Pause all animation
	void PauseAllAnimations( Bool flag );

	//! Is animations paused
	Bool IsAnyAnimationPaused() const;

public:
	Uint32 AddConstraint( IAnimatedSkeletonConstraint* constraint );
	Bool RemoveConstraint( Uint32 constraintId );

protected:
	//! Stop animation
	Bool StopAnimation( CSkeletalAnimationSetEntry *animation );

	//! Is playing animation
	Bool IsPlayingAnimation( CSkeletalAnimationSetEntry *animation ) const;

	//! Is playing animation
	Bool IsPlayingAnimation( const CName& animationName ) const;

	//! Get number of currently playing animations
	Uint32 GetNumPlayedAnimations() const;

protected:
	void BlendPoses( SBehaviorGraphOutput& output, const SBehaviorGraphOutput* pose, Float weight, ESkeletalAnimationType type ) const;
};

class CBehaviorAnimatedSkeleton : public CAnimatedSkeleton
{
public:
	CBehaviorAnimatedSkeleton( const CSkeleton *skeleton );
};

/*class CMimicAnimatedSkeleton : public CBehaviorAnimatedSkeleton
{
public:
	CMimicAnimatedSkeleton( const CSkeleton *skeleton );

	//! Play mimic animation on this skeleton
	CPlayedSkeletalAnimation* PlayMimicAnimation(	CSkeletalAnimationSetEntry *animation, 
													const CMimicFaces* mimicFaces );
};*/

//////////////////////////////////////////////////////////////////////////

class IAnimatedSkeletonConstraint
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Animation, MC_Animation );

public:
	virtual ~IAnimatedSkeletonConstraint(){}
	virtual void Update( const CAnimatedComponent* ac, Float dt ) = 0;
	virtual void PreSample( const CAnimatedComponent* ac, SBehaviorSampleContext* context, SBehaviorGraphOutput& pose ) = 0;
	virtual void PostSample( const CAnimatedComponent* ac, SBehaviorSampleContext* context, SBehaviorGraphOutput& pose ) = 0;
};
