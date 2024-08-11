
#pragma once

#include "expBaseExecutor.h"

/**
 *	Used to chain two executors together by using one animation (ending) and another one (starting)
 */
class ExpBlendChainExecutor : public ExpBaseExecutor
{
protected:
	IExpExecutor*						m_destExe;
	Float								m_currentTime;
	SAnimationState						m_animationStates[2];
	const CSkeletalAnimationSetEntry*	m_animationEntry[2];

public:

	ExpBlendChainExecutor( const ExecutorSetup& setup, IExpExecutor* destExe, const CName& animEnding, const CName& animStarting, Float blendTime = 0.1f );
	virtual ~ExpBlendChainExecutor();

protected:
	virtual void SyncAnim( Float time );
	virtual Bool UpdateAnimation( Float dt, Float& timeRest, ExpExecutorUpdateResult& result );
	virtual void Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result );
};
