
#pragma once

#include "animationTrajectoryUtils.h"

class CEdAnimationParamInitializer
{
public:
	virtual Bool Initialize( ISkeletalAnimationSetEntryParam* param, const CSkeletalAnimationSetEntry* animation, const CAnimatedComponent* animatedComponent ) const = 0;
};

//////////////////////////////////////////////////////////////////////////

