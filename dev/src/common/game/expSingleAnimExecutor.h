
#pragma once

#include "expBaseExecutor.h"

class ExpSingleAnimExecutor : public ExpBaseExecutor
{
protected:
	SAnimationState						m_animationState;
	const CSkeletalAnimationSetEntry*	m_animationEntry;
	Bool								m_endWhenBlendingOut;

public:
	ExpSingleAnimExecutor( const ExecutorSetup& setup, const CName& animName, Float blendIn = 0.f, Float blendOut = 0.f, Float earlyEndOffset = 0.f );
	~ExpSingleAnimExecutor();

	virtual void SyncAnim( Float time );

	// with this set to true, we will switch action immediately to something new from exploration action - giving control back to player WHILE blending out (as opposed to AFTER blending out)
	void EndWhenBlendingOut( Bool endWhenBlendingOut = true ) { m_endWhenBlendingOut = endWhenBlendingOut; m_blendOutOnEnd = m_blendOutOnEnd || endWhenBlendingOut; }
	const CSkeletalAnimationSetEntry* GetAnimationEntry(){ return m_animationEntry; }
protected:
	virtual Bool UpdateAnimation( Float dt, Float& timeRest, ExpExecutorUpdateResult& result );

	virtual void OnEnd( ExpExecutorUpdateResult& result );

#ifndef EXPLORATION_DEBUG
private:
#endif
	void CollectEvents( Float prev, Float curr, ExpExecutorUpdateResult& result ) const;
};