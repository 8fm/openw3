#pragma once

#include "expSlideExecutor.h"

class ExpMountBoatExecutor : public ExpSlideExecutor
{


public:
	ExpMountBoatExecutor( const IExploration* e, const IExplorationDesc* desc, const ExecutorSetup& setup, const CName& animName, Float blendIn = 0.f, Float blendOut = 0.f, Float earlyEndOffset = 0.f, Bool swapSide = false, Bool alignWhenCloseToEnd = false, Bool blockCloseToEnd = false, Bool alignTowardsInside = false );
	~ExpMountBoatExecutor();

	virtual void Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result ) override;

	virtual void GenerateDebugFragments( CRenderFrame* frame ) override;
};
