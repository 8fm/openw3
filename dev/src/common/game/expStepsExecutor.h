
#pragma once

#include "expIntarface.h"

class ExpBaseExecutor;
class ExpSlideExecutor;

//////////////////////////////////////////////////////////////////////////

class ExpStepsExecutor : public IExpExecutor
{
protected:
	enum InternalDir
	{
		ID_Positive,
		ID_Negative,
		ID_None,
	};

	const IExploration*	m_exploration;

private:
	enum InternalState
	{
        IS_LoopIdleStart,
		IS_LoopIdle,
		IS_LoopStep,
	} m_state;


	ExpBaseExecutor*	m_loopExe;

	ExpBaseExecutor*	m_stepP1Exe;
	ExpBaseExecutor*	m_stepP2Exe;
	ExpBaseExecutor*	m_stepN1Exe;
	ExpBaseExecutor*	m_stepN2Exe;

	IExpExecutor*		m_break1Exe;
	IExpExecutor*		m_break2Exe;

	//ExpSimpleSlider		m_slider;

    InternalDir			m_direction;
    InternalDir			m_prevDirection;
	Int32				m_startingSide; // 1 right hand up (beginning of left up anim), 0 left hand up (beginning of right up anim)

	ExpRelativeDirection m_queryCachedDirection;

public:
	ExpStepsExecutor();
	~ExpStepsExecutor();

	void ConnectSteps( const IExploration* e, const ExecutorSetup& setup, const CName& animation );
	void ConnectSteps( const IExploration* e, const ExecutorSetup& setup, const CName& animationP, const CName& animationN );
	void ConnectSteps( const IExploration* e, const ExecutorSetup& setup, const CName& animationP1, const CName& animationP2, const CName& animationN1, const CName& animationN2 );

	void ConnectBreak( IExpExecutor* exe );
	void ConnectBreak( IExpExecutor* exe1, IExpExecutor* exe2 );

	void SetStartingSide( Int32 side ) { m_startingSide = side; }

public:
	virtual void Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result );

	virtual void GenerateDebugFragments( CRenderFrame* frame );

protected:
	virtual Bool ContinueLoop( const ExpExecutorContext& context, InternalDir& dir ) = 0;
	virtual void ProcessGettingOff( ExpExecutorUpdateResult& result ) = 0;

	virtual Bool IsBreakRequest( const ExpExecutorContext& context ) const = 0;
	virtual Bool IsTransitionRequest( const ExpExecutorContext& context ) const = 0;

	virtual ExpRelativeDirection GetQueryDirection( const ExpExecutorContext& context ) const = 0;

	virtual void ProcessAdditives();

private:
	void StepAdjustmentPrepare();
	void StepAdjustmentUpdate();

	void CheckTransitions( const ExpExecutorContext& context, ExpExecutorUpdateResult& result );
	void ResetTransitions( const ExpExecutorContext& context, ExpExecutorUpdateResult& result );

	ExpBaseExecutor* ChooseLoopExe() const;
    void             GetSide(Int32& side) const;
	Bool ProcessBreak( const ExpExecutorContext& context, ExpExecutorUpdateResult& result );
};
