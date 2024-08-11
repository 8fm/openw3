/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
* Kamil Nowakowski
*/
#include "build.h"
#include "renderCollector.h"
#include "renderScene.h"
#include "renderCollectorTickList.h"

CRenderCollectorTickList::CRenderCollectorTickList()
	: m_currentCollectionCounter( 0 )
	, m_collectedTickAsyncArrayCounter( 0 )
	, m_collectionEnabled( true )
{

}

CRenderCollectorTickList::~CRenderCollectorTickList()
{
	// Thank you for heavy work
}


void CRenderCollectorTickList::PushProxy( IRenderProxyBase* proxy )
{
	if ( proxy == nullptr || !m_collectionEnabled )
	{
		return;
	}

	const Uint32 moduloMask = ASYNC_COLLECTED_TICK_SIZE-1;

	// Ring index. We are interested in previous value. New one marks end of the ring buffer with no valid element in there
	Uint32 index = ( m_collectedTickAsyncArrayCounter.Increment() - 1 ) & moduloMask;

	RED_ASSERT( index != ( m_currentCollectionCounter-1 ) , TXT("Collection ring buffer is too small. Consider incement the size of ASYNC_COLLECTED_TICK_SIZE") );
	RED_FATAL_ASSERT( m_collectedTickAsyncArray[ index ] == nullptr , "Writing pointer to stil pending one" );

	m_collectedTickAsyncArray[ index ] = proxy;
}


void CRenderCollectorTickList::TickCollectedProxies( CRenderSceneEx* scene )
{
	PC_SCOPE_RENDER_LVL1( CollectedProxiesTick );

	const Uint32 moduloMask = ASYNC_COLLECTED_TICK_SIZE-1;
	const Uint32 arrayTail = m_collectedTickAsyncArrayCounter.GetValue() & moduloMask;

	Uint32 index = m_currentCollectionCounter;
#ifdef RED_ASSERTS_ENABLED
	Uint32 countChecker = 0;
#endif
	// Super fast ring walk-through
	while( index != arrayTail )
	{
		IRenderProxyBase* volatile &cur = m_collectedTickAsyncArray[index];

		while( cur == nullptr )
		{ RED_FATAL_ASSERT(++countChecker < 0xFFFFFFFU, "This takes toooo much time" ); }

		cur->CollectedTick( scene );
		cur = nullptr;

		index = ( index+1 ) & moduloMask;
	}

	m_currentCollectionCounter = arrayTail;
}