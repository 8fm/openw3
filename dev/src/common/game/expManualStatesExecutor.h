
#pragma once

#include "expIntarface.h"
#include "expStatesExecutor.h"
#include "expStepsExecutor.h"

class ExpBaseExecutor;
class ExpSlideExecutor;

//////////////////////////////////////////////////////////////////////////

class ExpManualStepsExecutor : public ExpStepsExecutor
{
public:
	ExpManualStepsExecutor(  const IExploration* e, const IExplorationDesc* desc, const ExecutorSetup& setup  );

	void AutoGoInDir( Float inDir ) { m_autoGoInDir = inDir; }

protected:
	virtual Bool ContinueLoop( const ExpExecutorContext& context, InternalDir& dir );
	virtual void ProcessGettingOff( ExpExecutorUpdateResult& result );

	virtual Bool IsBreakRequest( const ExpExecutorContext& context ) const;
	virtual Bool IsTransitionRequest( const ExpExecutorContext& context ) const;

	virtual ExpRelativeDirection GetQueryDirection( const ExpExecutorContext& context ) const;

private:
	const CEntity*	m_entity;
	const IExplorationDesc* m_ExplorationDescription;
	Bool m_shouldGetOffAtBottom; // will store information that character should break off
	Float m_autoGoInDir; // used for AIs to traverse ladder

	void StepManualAdjustmentPrepare( Bool goingUp );
};

//////////////////////////////////////////////////////////////////////////

