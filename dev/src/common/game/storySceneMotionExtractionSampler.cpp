
#include "build.h"
#include "storySceneMotionExtractionSampler.h"
#include "storySceneIncludes.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

CStorySceneMotionExtractionSampler::CStorySceneMotionExtractionSampler( const CSkeletalAnimationSetEntry* animation )
	: m_animation( animation )
	, m_startTime( 0.f )
	, m_endTime( 0.f )
	, m_weight( 1.f )
	, m_blendIn( 0.f )
	, m_blendOut( 0.f )
	, m_stretch( 1.f )
	, m_clipFront( 0.f )
{
	SetTimes( 0.f, animation->GetDuration() );
}

void CStorySceneMotionExtractionSampler::SetTimes( Float startAnimTime, Float endAnimTime )
{
	m_startTime = startAnimTime;
	m_endTime = endAnimTime;

	SCENE_ASSERT( m_startTime <= m_endTime );
}

void CStorySceneMotionExtractionSampler::SetBlends( Float blendInDuration, Float blendOutDuration )
{
	// TODO blendInDur + blenOutDur > animDuration
	m_blendIn = blendInDuration;
	m_blendOut = blendOutDuration;
}

void CStorySceneMotionExtractionSampler::SetStretch( Float s )
{
	m_stretch = s;
}

void CStorySceneMotionExtractionSampler::SetClipFront( Float time )
{
	m_clipFront = time;
}

void CStorySceneMotionExtractionSampler::SetWeight( Float w )
{
	m_weight = w;
}

Float CStorySceneMotionExtractionSampler::GetWeight() const
{
	return m_weight;
}

Bool CStorySceneMotionExtractionSampler::Sample( Float sampleStartTime, Float sampleEndTime, AnimQsTransform& motionOut ) const
{
	SCENE_ASSERT( sampleStartTime <= sampleEndTime );

	const Float sampleStartTimeClamped = Clamp( sampleStartTime, m_startTime, m_endTime );
	const Float sampleEndTimeClamped = Clamp( sampleEndTime, m_startTime, m_endTime );
	
	Float prevTimeAnim = Max( 0.f, sampleStartTimeClamped - m_startTime );
	Float currTimeAnim = Max( 0.f, sampleEndTimeClamped - m_startTime );

	const Float animDuration = m_animation->GetDuration();

	prevTimeAnim /= m_stretch;
	currTimeAnim /= m_stretch;

	prevTimeAnim += m_clipFront;
	currTimeAnim += m_clipFront;

	SCENE_ASSERT( prevTimeAnim >= 0.f );
	SCENE_ASSERT( currTimeAnim >= 0.f );
	SCENE_ASSERT( prevTimeAnim - animDuration <= 0.001f );
	SCENE_ASSERT( currTimeAnim - animDuration <= 0.001f );

	prevTimeAnim = Clamp( prevTimeAnim, 0.f, animDuration );
	currTimeAnim = Clamp( currTimeAnim, 0.f, animDuration );

	const Bool isNotEqual = MAbs( prevTimeAnim - animDuration ) > FLT_EPSILON;

	if ( prevTimeAnim >= 0.f && currTimeAnim > 0.f && isNotEqual && prevTimeAnim < animDuration )
	{
		AnimQsTransform identity( AnimQsTransform::IDENTITY );
		motionOut = m_animation->GetAnimation()->GetMovementBetweenTime( prevTimeAnim, currTimeAnim, 0 );

		if ( prevTimeAnim > animDuration - m_blendOut )
		{
			const Float weight = ( ( prevTimeAnim - animDuration + m_blendOut ) / m_blendOut ) * m_weight;
			SCENE_ASSERT( weight >= 0.f && weight <= 1.f );
			motionOut.Lerp( identity, motionOut, weight );

		}
		else if ( currTimeAnim < m_blendIn )
		{
			const Float weight = ( ( m_blendIn - currTimeAnim ) / m_blendIn ) * m_weight;
			SCENE_ASSERT( weight >= 0.f && weight <= 1.f );
			motionOut.Lerp( identity, motionOut, weight );
		}
		else if ( m_weight < 1.f )
		{
			SCENE_ASSERT( m_weight >= 0.f && m_weight <= 1.f );
			motionOut.Lerp( identity, motionOut, m_weight );
		}

		return true;
	}
	else
	{
		motionOut.SetIdentity();
		return false;
	}
}

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif
