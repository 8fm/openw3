/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "animationBuffer.h"
#include "animMath.h"
#include "behaviorIncludes.h"

namespace ImportAnimationUtils
{

	void ConvertToAdditiveAnimation( const AnimQsTransform* tPoseTransform, Int32 tPoseTransformNum, IAnimationBuffer::SourceData::Part* animation, Int32 numBones, Int32 numTracks );
	void ConvertToAdditiveAnimation( IAnimationBuffer::SourceData::Part* animation, Int32 numBones, EAdditiveType type );

	void ConvertToAdditiveAnimation1( AnimQsTransform* buffer, Int32 numberOfTransformTracks, Int32 numberOfPoses );
	void ConvertToAdditiveAnimation2( AnimQsTransform* buffer, Int32 numberOfTransformTracks, Int32 numberOfPoses );

	void ConvertToAdditiveFrame( AnimQsTransform* firstFrame, AnimQsTransform* currFrame, Int32 numberOfTransformTracks, EAdditiveType type );
	void ConvertToAdditiveFrame1( AnimQsTransform* firstFrame, AnimQsTransform* currFrame, Int32 numberOfTransformTracks );
	void ConvertToAdditiveFrame2( AnimQsTransform* firstFrame, AnimQsTransform* currFrame, Int32 numberOfTransformTracks );

}
