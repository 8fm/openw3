
#pragma once

#include "expIntarface.h"

class ExpBaseExecutor;

class ExpChainExecutor : public IExpExecutor
{
	TDynArray< ExpBaseExecutor* >	m_executors;
	Int32								m_index;

public:
	ExpChainExecutor( const IExploration* e, const ExecutorSetup& setup, const TDynArray< CName >& anims );
	~ExpChainExecutor();

	virtual void Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result );

	virtual void GenerateDebugFragments( CRenderFrame* frame );

protected:
	void GoToNextAnimation( ExpExecutorUpdateResult& result );
};
