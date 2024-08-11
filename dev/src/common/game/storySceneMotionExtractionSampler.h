
#pragma once

#include "storySceneInstanceBuffer.h"

class CStoryScenePlayer;

class CStorySceneMotionExtractionSampler
{
	const CSkeletalAnimationSetEntry* m_animation;

	Float	m_startTime;
	Float	m_endTime;

	Float	m_clipFront;
	Float	m_stretch;

	Float	m_blendIn;
	Float	m_blendOut;

	Float	m_weight;

public:
	CStorySceneMotionExtractionSampler( const CSkeletalAnimationSetEntry* m_animation );

	void SetTimes( Float startTime, Float endTime );
	void SetBlends( Float blendInDuration, Float blendOutDuration );
	void SetWeight( Float w );
	void SetStretch( Float s );
	void SetClipFront( Float time );

public:
	Float GetWeight() const;

	Bool Sample( Float startTime, Float endTime, AnimQsTransform& motionOut ) const;
};
