
#pragma once

#include "expIntarface.h"
#include "expSlideExecutor.h"

class ExpBaseExecutor;
class ExpToPointSliderExecutor;

//////////////////////////////////////////////////////////////////////////

#if 0
RED_MESSAGE("FIXME>>>>> Make this compile!")

template< class TLoopExe >
class Exp2StatesExecutor : public IExpExecutor
{
	enum InternalState
	{
		IS_Pre,
		IS_Loop,
	} m_state;

	ExpSlideExecutor*	m_preExe;
	TLoopExe*			m_loopExe;

public:
	Exp2StatesExecutor( const IExploration* e, const ExecutorSetup& setup, const CName& pre, const CName& loop )
		: m_state( IS_Pre )
	{
		m_preExe = new ExpSlideExecutor( e, setup, pre );
		m_loopExe = new TLoopExe( e, setup, loop );
	}

	~Exp2StatesExecutor()
	{
		delete m_preExe;
		delete m_loopExe;
	}

	virtual void Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result )
	{
		ExpExecutorUpdateResult localResult;

		switch ( m_state )
		{
		case IS_Pre:
			{
				m_preExe->Update( context, result );

				if ( result.m_finished )
				{
					result.m_finished = false;

					m_state = IS_Loop;
				}
				else
				{
					break;
				}
			}


		case IS_Loop:
			{
				m_loopExe->Update( context, result );

				if ( result.m_finished )
				{
					result.m_finished = false;
				}

				break;
			}
		}
	}

	virtual void GenerateDebugFragments( CRenderFrame* frame )
	{
		switch ( m_state )
		{
		case IS_Pre:
			{
				m_preExe->GenerateDebugFragments( frame );
				break;
			}


		case IS_Loop:
			{
				m_loopExe->GenerateDebugFragments( frame );
				break;
			}
		}
	}
};

//////////////////////////////////////////////////////////////////////////

template< class TLoopExe >
class Exp3StatesExecutor : public IExpExecutor
{
	enum InternalState
	{
		IS_Pre,
		IS_Loop,
		IS_Post,
	} m_state;

	ExpSlideExecutor*	m_preExe;
	TLoopExe*			m_loopExe;
	ExpBaseExecutor*	m_postExe;

public:
	Exp3StatesExecutor( const IExploration* e, const ExecutorSetup& setup, const CName& pre, const CName& loop, const CName& post )
		: m_state( IS_Pre )
	{
		m_preExe = new ExpSlideExecutor( e, setup, pre );
		m_loopExe = new TLoopExe( e, setup, loop );
		m_postExe = new ExpBaseExecutor( e, setup, post );
	}

	~Exp3StatesExecutor()
	{
		delete m_preExe;
		delete m_loopExe;
		delete m_postExe;
	}

	virtual void Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result )
	{
		ExpExecutorUpdateResult localResult;

		switch ( m_state )
		{
		case IS_Pre:
			{
				m_preExe->Update( context, localResult );

				if ( localResult.m_finished )
				{
					localResult.m_finished = false;

					m_state = IS_Loop;
				}
				else
				{
					break;
				}
			}


		case IS_Loop:
			{
				if ( CanGoToPost( context ) )
				{
					m_state = IS_Post;
				}
				else
				{
					m_loopExe->Update( context, localResult );

					break;
				}
			}


		case IS_Post:
			{
				m_postExe->Update( context, localResult );

				if ( localResult.m_finished )
				{
					result.m_finished = true;
				}

				break;
			}
		}
	}

	virtual void GenerateDebugFragments( CRenderFrame* frame )
	{
		switch ( m_state )
		{
		case IS_Pre:
			{
				m_preExe->GenerateDebugFragments( frame );
				break;
			}


		case IS_Loop:
			{
				m_loopExe->GenerateDebugFragments( frame );
				break;
			}

		case IS_Post:
			{
				m_postExe->GenerateDebugFragments( frame );
				break;
			}
		}
	}

protected:
	virtual Bool CanGoToPost( const ExpExecutorContext& context ) const
	{
		return context.m_positionOnEdge > 0.99f;
	}
};

//////////////////////////////////////////////////////////////////////////

template< class TLoopExe >
class Exp4StatesExecutor : public IExpExecutor
{
	enum InternalState
	{
		IS_Pre,
		IS_Loop,
		IS_PostP,
		IS_PostN,
	} m_state;

	ExpToPointSliderExecutor*	m_preExe;
	TLoopExe*			        m_loopExe;
	ExpBaseExecutor*	        m_postPExe;
	ExpBaseExecutor*	        m_postNExe;

public:
	Exp4StatesExecutor( const IExploration* e, const ExecutorSetup& setup, const CName& pre, const CName& loop, const CName& postP, const CName& postN )
		: m_state( IS_Pre )
	{
		m_preExe = new ExpSlideExecutor( e, setup, pre );
		m_loopExe = new TLoopExe( e, setup, loop );
		m_postPExe = new ExpBaseExecutor( e, setup, postP );
		m_postNExe = new ExpBaseExecutor( e, setup, postN );
	}

	~Exp4StatesExecutor()
	{
		delete m_preExe;
		delete m_loopExe;
		delete m_postPExe;
		delete m_postNExe;
	}

	virtual void Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result )
	{
		ExpExecutorUpdateResult localResult;

		switch ( m_state )
		{
		case IS_Pre:
			{
				m_preExe->Update( context, localResult );

				if ( localResult.m_finished )
				{
					localResult.m_finished = false;

					m_state = IS_Loop;
				}
				else
				{
					break;
				}
			}


		case IS_Loop:
			{
				if ( CanGoToPostP( context ) )
				{
					m_state = IS_PostP;
				}
				else if ( CanGoToPostN( context ) )
				{
					m_state = IS_PostN;
				}
				else
				{
					m_loopExe->Update( context, localResult );

					break;
				}
			}


		case IS_PostP:
			{
				m_postPExe->Update( context, localResult );

				if ( localResult.m_finished )
				{
					result.m_finished = true;
				}

				break;
			}


		case IS_PostN:
			{
				m_postNExe->Update( context, localResult );

				if ( localResult.m_finished )
				{
					result.m_finished = true;
				}

				break;
			}
		}
	}

	virtual void GenerateDebugFragments( CRenderFrame* frame )
	{
		switch ( m_state )
		{
		case IS_Pre:
			{
				m_preExe->GenerateDebugFragments( frame );
				break;
			}

		case IS_Loop:
			{
				m_loopExe->GenerateDebugFragments( frame );
				break;
			}

		case IS_PostP:
			{
				m_postPExe->GenerateDebugFragments( frame );
				break;
			}

		case IS_PostN:
			{
				m_postNExe->GenerateDebugFragments( frame );
				break;
			}
		}
	}

protected:
	virtual Bool CanGoToPostP( const ExpExecutorContext& context ) const
	{
		return context.m_positionOnEdge > 0.99f;
	}

	virtual Bool CanGoToPostN( const ExpExecutorContext& context ) const
	{
		return context.m_positionOnEdge < 0.01f;
	}
};

#endif // #if 0

//////////////////////////////////////////////////////////////////////////

class ExpPreLoopPostExecutor : public IExpExecutor
{

	static const Float DISTANCE_TO_SWITCH_TO_POST;

	enum InternalState
	{
		IS_Loop,
		IS_Pre,
		IS_Post,
	} m_state;

	const CEntity*			m_entity;
	const IExploration*		m_exploration;
	const IExplorationDesc* m_explorationDesc;

    ExpToPointSliderExecutor*		m_preExe;
    ExpToPointSliderExecutor*		m_preExeL;
    ExpToPointSliderExecutor*		m_preExeR;
    IExpExecutor*			        m_loopExe;
    ExpToPointSliderExecutor*		m_postExe;
    ExpToPointSliderExecutor*		m_postExeL;
    ExpToPointSliderExecutor*		m_postExeR;
    ExpSlideExecutor*				m_postSlideExecutor;	
	Float							m_distanceToSwitchToPost;

public:
	ExpPreLoopPostExecutor( const IExploration* e, const IExplorationDesc* desc, const ExecutorSetup& setup );
	~ExpPreLoopPostExecutor();

	void ConnectPreLeft( ExpToPointSliderExecutor* exe );
    void ConnectPostLeft( ExpToPointSliderExecutor* exe );
    void ConnectPreRight( ExpToPointSliderExecutor* exe );
    void ConnectPostRight( ExpToPointSliderExecutor* exe );
    void ConnectLoop( IExpExecutor* exe );
	
	virtual void Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result );

	virtual void GenerateDebugFragments( CRenderFrame* frame );

	void CalculateAnimationZToSwitchToPost();

private:
	Int32 CanGoToPreOrPost() const;

	void SlideToPrePoint();
	void SlideToPostPoint();
};

//////////////////////////////////////////////////////////////////////////

class ExpPrePostExecutor : public IExpExecutor
{
public:
    enum InternalState
    {
        IS_Start,
        IS_Pre,
        IS_Post,
    };
private:
	InternalState m_state;

    const CEntity*			m_entity;
    const IExploration*		m_exploration;
    const IExplorationDesc* m_explorationDesc;

    ExpSlideExecutor*		m_preExe;
    ExpSlideExecutor*		m_postExe;

    ExpSlideExecutor*       m_postSlideExecutor;

public:
    ExpPrePostExecutor( const IExploration* e, const IExplorationDesc* desc, const ExecutorSetup& setup );
    ~ExpPrePostExecutor();

    void ConnectPre( ExpSlideExecutor* exe );
    void ConnectPost( ExpSlideExecutor* exe );
    
    virtual void Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result );

    virtual void GenerateDebugFragments( CRenderFrame* frame );
    Bool CalcSide() const;
	void ForceSide( InternalState side ){ m_state = side; }
private:

    void SlideToPrePoint();
    void SlideToPostPoint();
};

 