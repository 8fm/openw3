
#include "build.h"
#include "expChainExecutor.h"
#include "expSingleAnimExecutor.h"

ExpChainExecutor::ExpChainExecutor( const IExploration* e, const ExecutorSetup& setup, const TDynArray< CName >& anims )
	: m_index( 0 )
{
	const Uint32 size = anims.Size();
	m_executors.Resize( size );

	for ( Uint32 i=0; i<size; ++i )
	{
		m_executors[ i ] = new ExpSingleAnimExecutor( setup, anims[ i ] );
	}
}

ExpChainExecutor::~ExpChainExecutor()
{
	m_executors.ClearPtr();
}

void ExpChainExecutor::Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result )
{
	if ( m_executors.Size() == 0 )
	{
		result.m_finished = true;
		return;
	}

	ASSERT( m_index < m_executors.SizeInt() );

	ExpBaseExecutor* exe = m_executors[ m_index ];

	exe->Update( context, result );

	if ( result.m_finished && m_index+1 < m_executors.SizeInt() )
	{
		GoToNextAnimation( result );
		result.m_finished = false;
	}
}

void ExpChainExecutor::GoToNextAnimation( ExpExecutorUpdateResult& result )
{
	ASSERT( m_index+1 < m_executors.SizeInt() );

	m_index += 1;

	ExpBaseExecutor* exe = m_executors[ m_index ];

	ASSERT( result.m_timeRest >= 0.f );

	exe->SyncAnim( result.m_timeRest );
}

void ExpChainExecutor::GenerateDebugFragments( CRenderFrame* frame )
{
	if ( m_executors.Size() != 0 )
	{
		ExpBaseExecutor* exe = m_executors[ m_index ];
		exe->GenerateDebugFragments( frame );
	}
}
