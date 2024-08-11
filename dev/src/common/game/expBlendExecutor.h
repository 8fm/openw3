
#pragma once

#include "expBaseExecutor.h"

class ExpBlendExecutor : public ExpBaseExecutor
{
protected:
	Float								m_blendWeight;
	SAnimationState						m_animationStates[2];
	const CSkeletalAnimationSetEntry*	m_animationEntry[2];

public:

	ExpBlendExecutor( const ExecutorSetup& setup, const CName& anim1, const CName& anim2, Float weight, Float blendIn = 0.f, Float blendOut = 0.f, Float earlyEndOffset = 0.f );
	virtual ~ExpBlendExecutor();

	virtual void SyncAnim( Float time );

protected:
	virtual Bool UpdateAnimation( Float dt, Float& timeRest, ExpExecutorUpdateResult& result );

private:
	void CollectEvents( Float prev, Float curr, ExpExecutorUpdateResult& result ) const;
};
