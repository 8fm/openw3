
#include "build.h"
#include "expTransitionExecutor.h"

ExpBaseTransitionExecutor::ExpBaseTransitionExecutor( const IExploration* to, const IExplorationDesc* toDesc, const ExecutorSetup& setup, const CName& transAnim, IExpExecutor* destExe, Bool swapSide )
	: ExpSlideExecutor( to, toDesc, setup, transAnim, 0.2f, 0.f, 0.f, swapSide )
	, m_destExe( destExe )
{

}

void ExpBaseTransitionExecutor::Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result )
{
	ExpSlideExecutor::Update( context, result );

	if ( result.m_finished )
	{
		result.m_nextExe = m_destExe;
	}
}

//////////////////////////////////////////////////////////////////////////

ExpBreakTransition::ExpBreakTransition( const ExecutorSetup& setup, const CName& breakAnim, const CName& callEventOnEntityAtTheEnd, Float blendOut, Float earlyEndOffset )
	: ExpToPointSliderExecutor( nullptr, nullptr, setup, breakAnim, 0.0f, blendOut, earlyEndOffset )
	, m_callEventOnEntityAtTheEnd( callEventOnEntityAtTheEnd )
	, m_calledEvent( false )
{

}


void ExpBreakTransition::Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result )
{
	ExpBaseExecutor::Update( context, result );

	if ( result.m_finished )
	{
		OnEnd( result );
	}
}

void ExpBreakTransition::OnEnd( ExpExecutorUpdateResult& result )
{
	if ( ! m_calledEvent )
	{
		if ( m_entity && ! m_callEventOnEntityAtTheEnd.Empty() )
		{
			// I'm terribly sorry about that but we need to call event on entity to inform about break's end
			const_cast< CEntity* >(m_entity)->CallEvent( m_callEventOnEntityAtTheEnd );
		}
		m_calledEvent = true;
	}
	result.m_jumpEnd = true;
}
