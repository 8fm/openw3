/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "playedAnimation.h"
#include "behaviorGraphOutput.h"
#include "cacheBehaviorGraphOutput.h"
#include "skeletalAnimationEntry.h"
#include "skeletalAnimationSet.h"
#include "skeleton.h"
#include "component.h"
#include "behaviorGraphUtils.inl"

//////////////////////////////////////////////////////////////////////////

CPlayedSkeletalAnimation::CPlayedSkeletalAnimation()
	: m_animation( NULL )
	, m_internalBlendTime( 0.f )
	, m_timeDelta( 0.f )
	, m_isFinished( false )
	, m_removeWhenFinished( true )
{
}

CPlayedSkeletalAnimation::CPlayedSkeletalAnimation( const CSkeletalAnimationSetEntry* animation, const CSkeleton* skeleton )
	: m_animation( animation )
	, m_skeleton( skeleton )
	, m_weight( 0.f )
	, m_blendIn( 0.f )
	, m_blendOut( 0.f )
	, m_speed( 1.f )
	, m_isFinished( false )
	, m_isPaused( true )
	, m_isLooped( true )
	, m_removeWhenFinished( true )
	, m_autoFireEffects( false )
	, m_internalBlendTime( 0.f )
	, m_timeDelta( 0.f )
{
	Reset();
}

CPlayedSkeletalAnimation::~CPlayedSkeletalAnimation()
{
	m_listeners.Clear();
}

Bool CPlayedSkeletalAnimation::Play( Bool looped, Float weight, Float blendIn, Float blendOut, ESkeletalAnimationType type, Bool autoFireEffects )
{
	m_isLooped = looped;
	m_weight = weight;
	m_blendIn = blendIn;
	m_blendOut = blendOut;
	m_type = type;
	m_autoFireEffects = autoFireEffects;

	if ( IsValid() )
	{
		Unpause();
		OnAnimationStarted();
		return true;
	}
	else
	{
		return false;
	}
}

void CPlayedSkeletalAnimation::Stop()
{
	OnAnimationStopped();

	m_isFinished = true;
}

void CPlayedSkeletalAnimation::Reset()
{
	m_localTime			= 0.0f;
	m_prevTime			= 0.0f;
	m_loops				= 0;
	m_firstUpdate		= true;
}

Float CPlayedSkeletalAnimation::GetTime() const
{
	return m_localTime;
}

void CPlayedSkeletalAnimation::SetTime( Float time )
{
	m_localTime = time;
	m_prevTime = m_localTime;
	m_loops = 0;
}

Float CPlayedSkeletalAnimation::GetDuration() const
{
	return m_animation ? m_animation->GetDuration() : 0.f;
}

const CName& CPlayedSkeletalAnimation::GetName() const
{
	const CSkeletalAnimationSetEntry* anim = m_animation.Get();
	return anim ? anim->GetName() : CName::NONE;
}

void CPlayedSkeletalAnimation::GetSyncInfo( CSyncInfo &info ) const
{
	info.m_currTime = m_localTime;
	info.m_prevTime = m_prevTime;
	info.m_totalTime = GetDuration();
}

void CPlayedSkeletalAnimation::SynchronizeTo( const CSyncInfo &info )
{
	m_localTime = info.m_currTime;
	m_prevTime = info.m_prevTime;
}

void CPlayedSkeletalAnimation::AddAnimationListener( IPlayedAnimationListener *listener )
{
	m_listeners.PushBackUnique( listener );
}

void CPlayedSkeletalAnimation::RemoveAnimationListener( IPlayedAnimationListener *listener )
{
	m_listeners.Remove( listener );
}

Bool CPlayedSkeletalAnimation::IsValid() const
{
	return m_animation && m_animation->GetAnimation() ? true : false;
}

Bool CPlayedSkeletalAnimation::IsEqual( const CSkeletalAnimationSetEntry* animation ) const
{ 
	return m_animation == animation ? true : false; 
}

Bool CPlayedSkeletalAnimation::IsEqual( const CPlayedSkeletalAnimation* animation ) const
{ 
	return animation->IsEqual( m_animation.Get() ); 
}

Bool CPlayedSkeletalAnimation::IsEqual( const CName& animationName ) const
{ 
	const CSkeletalAnimationSetEntry* anim = m_animation.Get();
	return anim && anim->GetName() == animationName; 
}

void CPlayedSkeletalAnimation::Update( Float dt )
{
	if ( IsValid() && !m_isPaused )
	{
		m_prevTime = m_localTime;

		// Advance time
		m_localTime += dt * m_speed;
	
		// Reset loop counter
		m_loops = 0;

		if ( !m_animation || !m_animation->GetAnimation() )
		{
			ASSERT( !m_animation || !m_animation->GetAnimation() );
			return;
		}

		const CSkeletalAnimation* skAnimation = m_animation->GetAnimation();

		m_timeDelta = dt;

		Float animDuration = skAnimation->GetDuration();
		if ( m_localTime >= animDuration )
		{
			ASSERT( animDuration != 0.f, TXT("There's an animation \"%s\" in \"%s\" with zero time."),
				m_animation->GetAnimation()? m_animation->GetAnimation()->GetName().AsChar() : TXT("--"),
				m_animation->GetAnimSet()? m_animation->GetAnimSet()->GetDepotPath().AsChar() : TXT("--"));
			if ( m_isLooped && animDuration > 0.0f )
			{	
				while( m_localTime >= animDuration )
				{
					m_localTime -= animDuration;
					++m_loops;					
				}
				if (m_speed >= 0.0f) // just in case we would start at the end for any reason
				{
					OnAnimationFinished();
				}
			}
			else
			{
				m_localTime = animDuration;
				if ( !m_isFinished )
				{
					if (m_speed >= 0.0f) // just in case we would start at the end for any reason
					{
						OnAnimationFinished();
					}
					m_isFinished = true;
				}
			}
		}

		if ( m_localTime < 0.0f )
		{
			if ( m_isLooped )
			{
				while( m_localTime < 0.0f )
				{
					m_localTime += animDuration;
					--m_loops;					
				}

				if (m_speed < 0.0f) // just in case we would start before start for any reason
				{
					OnAnimationFinished();
				}
			}
			else
			{
				m_localTime = 0.0f;
				if ( !m_isFinished )
				{
					if (m_speed < 0.0f) // just in case we would start before start for any reason
					{
						OnAnimationFinished();
					}
					m_isFinished = true;
				}
			}
		}

		if ( m_firstUpdate )
		{
			// Reset flag
			m_firstUpdate = false;
		}
	}
}

Bool CPlayedSkeletalAnimation::Sample( SBehaviorSampleContext* context, SBehaviorGraphOutput& pose ) const
{
	Bool ret = false;
#ifdef USE_HAVOK_ANIMATION
	pose.m_deltaReferenceFrameLocal.setIdentity();
#else
	pose.m_deltaReferenceFrameLocal.SetIdentity();
#endif

	if ( IsValid() )
	{
		const CSkeletalAnimation* skAnimation = m_animation->GetAnimation();
		const CSkeleton* skeleton = m_skeleton.Get();

		UpdateAndSampleBlendWithCompressedPose( m_animation.Get(), m_timeDelta, m_internalBlendTime, m_localTime, *context, pose, skeleton );

		pose.Touch();

		if ( !m_isPaused && skAnimation->HasExtractedMotion() )
		{
			pose.m_deltaReferenceFrameLocal = skAnimation->GetMovementBetweenTime( m_prevTime, m_localTime, m_loops );
		}
	}

	return ret;
}

Bool CPlayedSkeletalAnimation::Sample( SBehaviorGraphOutput& pose ) const
{
	Bool ret = false;
#ifdef USE_HAVOK_ANIMATION
	pose.m_deltaReferenceFrameLocal.setIdentity();
#else
	pose.m_deltaReferenceFrameLocal.SetIdentity();
#endif
	if ( IsValid() )
	{
		const CSkeletalAnimation* skAnimation = m_animation->GetAnimation();
		const CSkeleton* skeleton = m_skeleton.Get();

		Uint32 bonesLoaded;
		Uint32 bonesAlwaysLoaded;
		EAnimationBufferDataAvailable abda = skAnimation->GetAnimationBufferDataAvailable( pose.m_numBones, bonesLoaded, bonesAlwaysLoaded );
		if ( abda == ABDA_All )
		{
			ret = skAnimation->Sample( m_localTime, 
				pose.m_numBones,
				pose.m_numFloatTracks,
				pose.m_outputPose, 
				pose.m_floatTracks );
		}
		else if ( abda == ABDA_None )
		{
			ret = skAnimation->SampleCompressedPose( pose.m_numBones, pose.m_outputPose, pose.m_numFloatTracks, pose.m_floatTracks, skeleton );
		}
		else if ( abda == ABDA_Partial )
		{
			// fill with compressed pose first
			ret = skAnimation->SampleCompressedPose( pose.m_numBones, pose.m_outputPose, pose.m_numFloatTracks, pose.m_floatTracks, skeleton );
			// update as much as you can with sampled
			ret = skAnimation->Sample( m_localTime, 
				bonesLoaded,
				pose.m_numFloatTracks,
				pose.m_outputPose, 
				pose.m_floatTracks );
		}

		pose.Touch();
	}

	return ret;
}

void CPlayedSkeletalAnimation::CollectEvents( const CComponent * component, SBehaviorGraphOutput& pose ) const
{
	if ( IsValid() )
	{
		const Float eventsAlpha = 1.0f;
		const CName sfxTag = 
#ifdef USE_EXT_ANIM_EVENTS
			component && component->GetEntity() ? component->GetEntity()->GetSfxTag() : CName::NONE;
#else
			CName::NONE;
#endif

		m_animation->GetEventsByTime( m_prevTime, m_localTime, m_loops, eventsAlpha, NULL, &pose, sfxTag );
	}
}

Uint32 CPlayedSkeletalAnimation::GetBonesNum() const
{
	return IsValid() ? m_animation->GetAnimation()->GetBonesNum() : 0;
}

Uint32 CPlayedSkeletalAnimation::GetTracksNum() const
{
	return IsValid() ? m_animation->GetAnimation()->GetTracksNum() : 0;
}

Bool CPlayedSkeletalAnimation::HasBoundingBox() const
{ 
	return IsValid() ? m_animation->GetAnimation()->HasBoundingBox() : false; 
}

Box CPlayedSkeletalAnimation::GetBoundingBox() const
{ 
	return IsValid() ? m_animation->GetAnimation()->GetBoundingBox() : Box(); 
}

void CPlayedSkeletalAnimation::OnAnimationStopped() const
{
	for ( Uint32 i=0; i<m_listeners.Size(); ++i )
	{
		m_listeners[i]->OnAnimationStopped( this );
	}
}

void CPlayedSkeletalAnimation::OnAnimationStarted() const
{
	for ( Uint32 i=0; i<m_listeners.Size(); ++i )
	{
		m_listeners[i]->OnAnimationStarted( this );
	}
}

void CPlayedSkeletalAnimation::OnAnimationFinished() const
{
	for ( Uint32 i=0; i<m_listeners.Size(); ++i )
	{
		m_listeners[i]->OnAnimationFinished( this );
	}
}

#ifndef NO_EDITOR

void CPlayedSkeletalAnimation::ForceCompressedPose()
{
	m_internalBlendTime = BehaviorUtils::GetTimeFromCompressedBlend( m_animation->GetCompressedPoseBlend() ) + 0.2f;
}

#endif

//////////////////////////////////////////////////////////////////////////

/*CPlayedMimicSkeletalAnimation::CPlayedMimicSkeletalAnimation(	CSkeletalAnimationSetEntry* animation,
																const CSkeleton* skeleton,
																const CMimicFaces* mimicFaces )
	: CPlayedSkeletalAnimation( animation, skeleton )
	, m_mimicFaces( mimicFaces )
{

}

void CPlayedMimicSkeletalAnimation::SetTPose( const CSkeleton* skeleton, SBehaviorGraphOutput& tPose ) const
{
	const hkaSkeleton* havokSkeleton = skeleton->GetHavokSkeleton();
	ASSERT( havokSkeleton );

	Int32 size = Min( (Int32)tPose.m_numBones, havokSkeleton->m_numReferencePose );

	for ( Int32 i=0; i<size; ++i )
	{
		tPose.m_outputPose[ i ] = havokSkeleton->m_referencePose[ i ];
	}
}

Bool CPlayedMimicSkeletalAnimation::Sample( SBehaviorSampleContext* context, SBehaviorGraphOutput& pose ) const
{
	Bool ret = false;

	const CMimicFaces* mimicFaces = m_mimicFaces.Get();

	if ( mimicFaces && IsValid() && context->HasMimic() )
	{
		CCacheBehaviorGraphOutput cachePose( *context, true );
		SBehaviorGraphOutput* mimicPose = cachePose.GetPose();

		if ( mimicPose && TRACK_JAW < mimicPose->m_numFloatTracks )
		{
			mimicPose->m_deltaReferenceFrameLocal.setIdentity();

			ret = m_animation->GetAnimation()->Sample( GetTime(), mimicPose->m_numBones, mimicPose->m_numFloatTracks, mimicPose->m_outputPose, mimicPose->m_floatTracks );
			mimicPose->Touch();

			Float eyes = Clamp( mimicPose->m_floatTracks[ TRACK_EYES ], 0.f, 1.f );
			Float jaw = Clamp( mimicPose->m_floatTracks[ TRACK_JAW ], 0.f, 1.f );

			SBehaviorGraphOutput poseAddEyes;
			SBehaviorGraphOutput poseAddJaw;

			const CSkeleton* skeleton = mimicFaces->GetMimicSkeleton();
			ASSERT( skeleton );

			SetTPose( skeleton, pose );

			if ( eyes > 0.f && mimicFaces->GetMimicPose( LOW_POSE_EYES_NUM, poseAddEyes ) )
			{
				// Add eyes
				pose.SetAddMul( poseAddEyes, eyes );
			}

			if ( jaw > 0.f && mimicFaces->GetMimicPose( LOW_POSE_JAW_NUM, poseAddJaw ) )
			{
				// Add jaw
				pose.SetAddMul( poseAddJaw, jaw );
			}

			pose.Touch();
		}
	}

	return ret;
}*/
