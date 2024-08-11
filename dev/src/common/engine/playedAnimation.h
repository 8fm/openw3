/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "animSyncInfo.h"

class CPlayedSkeletalAnimation;
class CSkeletalAnimationSetEntry;
class CSkeleton;
class CSkeletalAnimationSetEntry;
class CSkeletalAnimation;
struct SBehaviorSampleContext;
struct SBehaviorGraphOutput;
enum ESkeletalAnimationType : CEnum::TValueType;

class IPlayedAnimationListener
{
public:
	virtual void OnAnimationStarted( const CPlayedSkeletalAnimation* /*animation*/ )			{}
	virtual void OnAnimationBlendInFinished( const CPlayedSkeletalAnimation* /*animation*/ )	{}
	virtual void OnAnimationBlendOutStarted( const CPlayedSkeletalAnimation* /*animation*/ )	{}
	virtual void OnAnimationFinished( const CPlayedSkeletalAnimation* /*animation*/ )			{}
	virtual void OnAnimationStopped( const CPlayedSkeletalAnimation* /*animation*/ )			{}
};

class CPlayedSkeletalAnimation
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Animation );

protected:
	ESkeletalAnimationType			m_type;

	THandle< CSkeleton >			m_skeleton;
	THandle< CSkeletalAnimationSetEntry > m_animation;
	Float							m_localTime;
	Float							m_prevTime;
	Int32							m_loops;
	mutable Float					m_internalBlendTime; // I am terribly sorry about mutable here - due to changes with animations and streaming and to make it easier and not call loading animation couple of times, code was simplified and made reusable and internalBlendTime is updated within Sample now
	Float							m_speed;
	Float							m_timeDelta;

	Bool							m_isFinished;
	Bool							m_isPaused;
	Bool							m_isLooped;
	Bool							m_removeWhenFinished;

	Float							m_weight;
	Float							m_blendIn;
	Float							m_blendOut;

	Bool							m_firstUpdate;
	Bool							m_autoFireEffects;

	TDynArray< IPlayedAnimationListener* > m_listeners;

public:
	CPlayedSkeletalAnimation();
	CPlayedSkeletalAnimation( const CSkeletalAnimationSetEntry* animation, const CSkeleton* skeleton );
	virtual ~CPlayedSkeletalAnimation();

public:
	RED_INLINE Float GetSpeed() const				{ return m_speed; }
	RED_INLINE Float GetWeight() const			{ return m_weight; }
	RED_INLINE Bool IsPaused() const				{ return m_isPaused; }
	RED_INLINE Bool IsLooped() const				{ return m_isLooped; }

	RED_INLINE void SetSpeed( Float speed )		{ m_speed = speed; }
	RED_INLINE void SetLooped( Bool looped )		{ m_isLooped = looped; }
	RED_INLINE void SetWeight( Float weight )		{ m_weight = weight; }
	RED_INLINE void Pause()						{ m_isPaused = true; }
	RED_INLINE void Unpause()						{ m_isPaused = false; }
	RED_INLINE void RemoveWhenFinished( Bool b )	{ m_removeWhenFinished = b; }
	RED_INLINE Bool ShouldFireAnimEffect() const	{ return m_firstUpdate && m_autoFireEffects; }
	RED_INLINE Bool ShouldBeRemoved() const		{ return m_isFinished && m_removeWhenFinished; }

	RED_INLINE ESkeletalAnimationType				GetType() const		{ return m_type; }
	RED_INLINE const CSkeletalAnimationSetEntry*	GetAnimationEntry() const { return m_animation.Get(); }

public:
	Bool Play( Bool looped, Float weight, Float blendIn, Float blendOut, ESkeletalAnimationType type, Bool autoFireEffects );
	void Stop();
	void Reset();

	void Update( Float dt );
	virtual Bool Sample( SBehaviorSampleContext* context, SBehaviorGraphOutput& output ) const;
	virtual Bool Sample( SBehaviorGraphOutput& output ) const;
	void CollectEvents( const CComponent * component, SBehaviorGraphOutput& pose ) const;

	Uint32 GetBonesNum() const;
	Uint32 GetTracksNum() const;

	Bool HasBoundingBox() const;
	Box GetBoundingBox() const;

public:
	Float GetTime() const;
	void SetTime( Float time );
	Float GetDuration() const;
	const CName& GetName() const;

	Bool IsValid() const;
	Bool IsEqual( const CSkeletalAnimationSetEntry* animation ) const;
	Bool IsEqual( const CPlayedSkeletalAnimation* animation ) const;
	Bool IsEqual( const CName& animationName ) const;

public:
	void GetSyncInfo( CSyncInfo &info ) const;
	void SynchronizeTo( const CSyncInfo &info );

public:
	void AddAnimationListener( IPlayedAnimationListener *listener );
	void RemoveAnimationListener( IPlayedAnimationListener *listener );

	void OnAnimationStopped() const;
	void OnAnimationStarted() const;
	void OnAnimationFinished() const;

#ifndef NO_EDITOR
public:
	void ForceCompressedPose();
#endif
};

//////////////////////////////////////////////////////////////////////////

/*class CPlayedMimicSkeletalAnimation : public CPlayedSkeletalAnimation
{
protected:
	THandle< CMimicFaces > m_mimicFaces;

	static const Uint32			LOW_POSE_REF_NUM = 0;
	static const Uint32			LOW_POSE_JAW_NUM = 1;
	static const Uint32			LOW_POSE_EYES_NUM = 2;
	static const Uint32			TRACK_JAW = 0;
	static const Uint32			TRACK_EYES = 26;

public:
	CPlayedMimicSkeletalAnimation( CSkeletalAnimationSetEntry* animation, const CSkeleton* skeleton, const CMimicFaces* mimicFaces );

public:
	virtual Bool Sample( SBehaviorSampleContext* context, SBehaviorGraphOutput& output );

protected:
	void SetTPose( const CSkeleton* skeleton, SBehaviorGraphOutput& tPose ) const;
};*/
