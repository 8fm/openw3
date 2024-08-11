/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "umbraQuery.h"

#ifdef USE_UMBRA

CPool UmbraQuery::s_queryPool;
void* UmbraQuery::s_queryMemory = NULL;

///////////////////////////////////////////////////////////////////////////////
// UmbraQuery
///////////////////////////////////////////////////////////////////////////////
void* UmbraQuery::operator new( size_t size )
{
	ASSERT( size == sizeof( UmbraQuery ) );
	return s_queryPool.AllocateBlock();
}
void UmbraQuery::operator delete( void* ptr )
{
	s_queryPool.FreeBlock( ptr );
}

void UmbraQuery::InitializeQueriesPool( Uint32 maxQueriesCount )
{
	ASSERT( !s_queryMemory,TXT("UmbraQuery memory was initialized multiple times") );
	s_queryMemory = RED_MEMORY_ALLOCATE( MemoryPool_Umbra, MC_UmbraQuery, sizeof( UmbraQuery ) * maxQueriesCount );
	s_queryPool.Init( s_queryMemory, maxQueriesCount * sizeof( UmbraQuery ), sizeof( UmbraQuery ) );
}
void UmbraQuery::ShutdownQueriesPool()
{
	RED_MEMORY_FREE( MemoryPool_Umbra, MC_UmbraQuery, s_queryMemory );
	s_queryMemory = nullptr;
}


///////////////////////////////////////////////////////////////////////////////
// UmbraQueryBatch
///////////////////////////////////////////////////////////////////////////////
UmbraQueryPtr UmbraQueryBatch::AddQuery( const Box& bbox )
{
	UmbraQuery* query = new UmbraQuery( bbox, m_queryList );
	if ( !query )
	{
		// its possible since queries use special memory pools
		return nullptr;
	}
	// manually add reference as we now store it on our internal list
	query->AddRef();
	m_queryList = query;
	// return smart pointer
	return UmbraQueryPtr( query );
}
void UmbraQueryBatch::Clear()
{
	UmbraQuery* query = m_queryList;
	while ( query )
	{
		UmbraQuery* nextQuery = query->m_next;
		query->Release();
		query = nextQuery;
	}
	m_queryList = nullptr;
}
void UmbraQueryBatch::CutBefore( Iterator it )
{
	UmbraQuery* cutQuery = &(*it);
	UmbraQuery* query = m_queryList;
	while ( query != cutQuery  )
	{
		UmbraQuery* nextQuery = query->m_next;
		query->Release();
		query = nextQuery;
	}
	m_queryList = cutQuery;
}

#endif // USE_UMBRA