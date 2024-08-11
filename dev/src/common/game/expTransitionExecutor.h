
#pragma once

#include "expIntarface.h"
#include "expSlideExecutor.h"
#include "expToPointSliderExecutor.h"

class ExpBaseTransitionExecutor : public ExpSlideExecutor
{
	IExpExecutor* m_destExe;

public:
	ExpBaseTransitionExecutor( const IExploration* to, const IExplorationDesc* toDesc, const ExecutorSetup& setup, const CName& transAnim, IExpExecutor* destExe, Bool swapSide = false );

	virtual void Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result );
};

//////////////////////////////////////////////////////////////////////////

class ExpBreakTransition : public ExpToPointSliderExecutor
{
public:
	ExpBreakTransition( const ExecutorSetup& setup, const CName& breakAnim, const CName& callEventOnEntityAtTheEnd, Float blendOut, Float earlyEndOffset = 0.0f );

	virtual void Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result );

protected:
	virtual void OnEnd( ExpExecutorUpdateResult& result );

private:
	CName m_callEventOnEntityAtTheEnd;
	Bool m_calledEvent;
};
