
#include "build.h"
#include "expStepsExecutor.h"
#include "expSingleAnimExecutor.h"

//////////////////////////////////////////////////////////////////////////

ExpStepsExecutor::ExpStepsExecutor()
	: m_state( IS_LoopIdle )
	, m_exploration( NULL )
	, m_stepP1Exe( NULL )
	, m_stepP2Exe( NULL )
	, m_stepN1Exe( NULL )
	, m_stepN2Exe( NULL )
	, m_loopExe( NULL )
	, m_break1Exe( NULL )
	, m_break2Exe( NULL )
	, m_direction( ID_None )
	, m_startingSide( 0 )
	, m_queryCachedDirection( ERD_None )
{
}

void ExpStepsExecutor::ConnectSteps( const IExploration* e, const ExecutorSetup& setup, 
	const CName& animation )
{
	m_exploration = e;

	m_stepP1Exe = new ExpSingleAnimExecutor( setup, animation );
	m_stepP2Exe = new ExpSingleAnimExecutor( setup, animation );
	m_stepN1Exe = new ExpSingleAnimExecutor( setup, animation );
	m_stepN2Exe = new ExpSingleAnimExecutor( setup, animation );

	m_stepN1Exe->SetTimeMul( -1.f );
	m_stepN2Exe->SetTimeMul( -1.f );

	m_loopExe = NULL;
}

void ExpStepsExecutor::ConnectSteps( const IExploration* e, const ExecutorSetup& setup, 
	const CName& animationP, const CName& animationN )
{
	m_exploration = e;

	m_stepP1Exe = new ExpSingleAnimExecutor( setup, animationP );
	m_stepP2Exe = new ExpSingleAnimExecutor( setup, animationP );
	m_stepN1Exe = new ExpSingleAnimExecutor( setup, animationN );
	m_stepN2Exe = new ExpSingleAnimExecutor( setup, animationN );

	m_loopExe = NULL;
}

void ExpStepsExecutor::ConnectSteps( const IExploration* e, const ExecutorSetup& setup, 
	const CName& animationP1, const CName& animationP2, const CName& animationN1, const CName& animationN2 )
{
	m_exploration = e;

	m_stepP1Exe = new ExpSingleAnimExecutor( setup, animationP1 );
	m_stepP2Exe = new ExpSingleAnimExecutor( setup, animationP2 );
	m_stepN1Exe = new ExpSingleAnimExecutor( setup, animationN1 );
	m_stepN2Exe = new ExpSingleAnimExecutor( setup, animationN2 );

	m_loopExe = NULL;
}

void ExpStepsExecutor::ConnectBreak( IExpExecutor* exe )
{
	ASSERT( !m_break1Exe );
	ASSERT( !m_break2Exe );

	m_break1Exe = exe;
}

void ExpStepsExecutor::ConnectBreak( IExpExecutor* exe1, IExpExecutor* exe2 )
{
	ASSERT( !m_break1Exe );
	ASSERT( !m_break2Exe );

	m_break1Exe = exe1;
	m_break2Exe = exe2;
}

ExpStepsExecutor::~ExpStepsExecutor()
{
	delete m_stepP1Exe;
	delete m_stepP2Exe;
	delete m_stepN1Exe;
	delete m_stepN2Exe;

	delete m_break1Exe;
	delete m_break2Exe;
}

void ExpStepsExecutor::Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result )
{
	Bool running = true;

	while ( running )
	{
		switch ( m_state )
		{

		case IS_LoopIdle:
			{
				ProcessBreak( context, result );

				if ( ContinueLoop( context, m_direction ) )
				{
                    if( (m_direction == ID_Positive && result.m_finishPoint == 1) 
                        || (m_direction == ID_Negative && result.m_finishPoint == -1))
                    {
                        result.m_finished = true;
                        running = false;

                        GetSide(result.m_side);

                        break;
                    }

					ASSERT( m_direction != ID_None );

					ResetTransitions( context, result );

					m_loopExe = ChooseLoopExe();

					ASSERT( m_loopExe );
					ASSERT( result.m_timeRest >= 0.f );
					m_loopExe->SyncAnim( result.m_timeRest );

					StepAdjustmentPrepare();
					m_state = IS_LoopStep;
                    m_prevDirection = m_direction;

				}
				else
				{
					ASSERT( m_direction == ID_None );
					/*
					if ( !ProcessBreak( context, result ) )
					{
						CheckTransitions( context, result );
					}
					*/
					ProcessAdditives();

					ExpBaseExecutor* idleExe = m_loopExe ? m_loopExe : (m_startingSide == 0? m_stepP1Exe : m_stepP2Exe);
					idleExe->SyncAnimToEnd();
                    
                    if( (m_prevDirection == ID_Positive && result.m_finishPoint == 1) 
                        || (m_prevDirection == ID_Negative && result.m_finishPoint == -1))
                    {
                        result.m_finished = false;
                    }

                    running = false;

                    GetSide(result.m_side);
				}

				ProcessGettingOff( result );
				break;
			}


		case IS_LoopStep:
			{
				ASSERT( m_direction != ID_None );
				ASSERT( m_queryCachedDirection == ERD_None );
				ASSERT( m_loopExe );

				ProcessBreak( context, result );

				m_loopExe->Update( context, result );
				StepAdjustmentUpdate();

				ProcessAdditives();

				if ( result.m_finished )
				{
					// Reset
					//localResult.m_finished = false;

                    m_state = IS_LoopIdle;

                    if( (m_direction == ID_Positive && result.m_finishPoint == -1) 
                        || (m_direction == ID_Negative && result.m_finishPoint == 1))
                    {
                        result.m_finished = false;
                    }
				}
				else
				{
					running = false;
				}

				break;
			}
		}
	}
}

void ExpStepsExecutor::StepAdjustmentPrepare()
{
	/*
	m_direction;
	m_loopExe->m_entity;
	m_explorationDesc->UseEdgeGranularity( granularity );
	if( granularity > 0.0f )
	{
		m_exploration->SetPointOnClosestGrain( p2, granularity );
	}
	*/
	//m_slider.Setup( )
}

void ExpStepsExecutor::StepAdjustmentUpdate()
{
	/*
	AnimQsTransform motion( AnimQsTransform::IDENTITY );
	AnimQuaternion remainingExRot( AnimQuaternion::IDENTITY );
	AnimVector4 remainingExTrans;
	const ESliderResult slided = m_slider.Update( motion, remainingExRot, remainingExTrans, m_animationState.m_prevTime, m_animationState.m_currTime, m_entity, m_pointOnEdge, m_targetYaw + m_targetYawOffset, m_alignRotToEdgeExceeding, allowPostEndTransAdjustment );
	*/
}

void ExpStepsExecutor::GenerateDebugFragments( CRenderFrame* frame )
{
	if ( m_loopExe )
	{
		m_loopExe->GenerateDebugFragments( frame );
	}

	//m_slider.GenerateDebugFragments( frame );
}

void ExpStepsExecutor::ProcessAdditives()
{

}

ExpBaseExecutor* ExpStepsExecutor::ChooseLoopExe() const
{
	ASSERT( m_direction != ID_None );

	if ( m_loopExe == NULL && m_direction == ID_Positive)
	{
		return m_direction == ID_Positive ? (m_startingSide == 0? m_stepP2Exe : m_stepP1Exe) : (m_startingSide == 0? m_stepN2Exe : m_stepN1Exe);
	}
	else if ( m_direction == ID_Positive )
	{
		return m_loopExe == m_stepP1Exe || m_loopExe == m_stepN2Exe ? m_stepP2Exe : m_stepP1Exe;
	}
	else // m_direction == ID_Negative
	{
		return m_loopExe == m_stepP2Exe || m_loopExe == m_stepN1Exe ? m_stepN2Exe : m_stepN1Exe;
	}
}

Bool ExpStepsExecutor::ProcessBreak( const ExpExecutorContext& context, ExpExecutorUpdateResult& result )
{
	if ( m_break1Exe )
	{
		const Bool isBreakRequest = IsBreakRequest( context );
		if ( isBreakRequest )
		{
            if(!m_loopExe && result.m_finishPoint != 0)
            {
                GetSide(result.m_side);
                result.m_finished = 1;
                return true;
            }

			const Bool break1 = !m_loopExe || m_loopExe == m_stepP1Exe || m_loopExe == m_stepN2Exe;
			if ( break1 )
			{
				result.m_nextExe = m_break1Exe;
				m_break1Exe = NULL;
				return true;
			}
			else
			{
				result.m_nextExe = m_break2Exe;
				m_break2Exe = NULL;
				return true;
			}
		}
	}

	return false;
}

void ExpStepsExecutor::ResetTransitions( const ExpExecutorContext& context, ExpExecutorUpdateResult& result )
{
	m_queryCachedDirection = ERD_None;

	if ( result.m_nnQueryRequest.QueryRequester( m_exploration ) )
	{
		result.m_nnQueryRequest.Reset();
	}
	else
	{
		ASSERT( !result.m_nnQueryRequest.m_from );
	}
}

void ExpStepsExecutor::CheckTransitions( const ExpExecutorContext& context, ExpExecutorUpdateResult& result )
{
	const ExpRelativeDirection dir = GetQueryDirection( context );

	const Bool transitionRequest = IsTransitionRequest( context );
	if ( transitionRequest && dir != ERD_None )
	{
		m_queryCachedDirection = dir;

		result.m_nnQueryRequest.QueryRequest( m_exploration, m_queryCachedDirection );
	}
	else if ( m_queryCachedDirection != ERD_None && ( dir == ERD_None || dir != m_queryCachedDirection ) )
	{
		ResetTransitions( context, result );
	}
	else if ( m_queryCachedDirection != ERD_None )
	{
		result.m_nnQueryRequest.QueryRequest( m_exploration, m_queryCachedDirection );
	}
}

void ExpStepsExecutor::GetSide( Int32& side ) const
{
    if ( !m_loopExe )
    {
        side = m_startingSide;
    }
    else 
	{
		// we're interested in pose at the end of animation
		// 0 left hand up
		// 1 right hand up
		side = m_loopExe == m_stepP1Exe ? 0 : m_loopExe == m_stepP2Exe ? 1 :
			   m_loopExe == m_stepN1Exe ? 1 : m_loopExe == m_stepN2Exe ? 0 : m_startingSide;
    }

}

