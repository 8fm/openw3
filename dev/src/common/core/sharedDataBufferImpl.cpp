/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "sharedDataBuffer.h"
#include "sharedDataBufferImpl.h"

//------------------

SharedDataBufferData::SharedDataBufferData( const void* sourceData, const Uint32 sourceSize, const Uint64 hash )
	: m_size( sourceSize )
	, m_hash( hash )
{
	RED_FATAL_ASSERT( sourceSize > 0, "Invalid size of data buffer" );
	RED_FATAL_ASSERT( sourceData != nullptr, "Invalid size of data buffer" );

	m_data = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_SharedBuffer, sourceSize );
	Red::MemoryCopy( m_data, sourceData, sourceSize );
}

SharedDataBufferData::~SharedDataBufferData()
{
	RED_FATAL_ASSERT( m_data != nullptr, "Invalid buffer being freed" );
	RED_MEMORY_FREE( MemoryPool_Default, MC_SharedBuffer, m_data );

	m_data = (void*)0xDEADF00D;
	m_size = 0xDEADF00D;
	m_hash = 0xDEADF00D;
}

//------------------

SharedDataBufferCache::SharedDataBufferCache()
{
}

SharedDataBufferCache::~SharedDataBufferCache()
{
}

SharedDataBufferData* SharedDataBufferCache::Request( const void* sourceData, const Uint32 sourceDataSize )
{
	// empty data
	if ( !sourceData || !sourceDataSize )
		return nullptr;

	// calculate data hash
	const Uint64 hash = CalcDataHash( sourceData, sourceDataSize );

	{
		TCacheScopeLock lock( m_lock );

		// use existing if possible
		Token* token = m_cache.FindPtr( hash );
		if ( token )
		{
			token->m_numRefs += 1;
			return token->m_data;
		}

		// create new entry
		Token newToken;
		newToken.m_numRefs = 1;
		newToken.m_data = new SharedDataBufferData( sourceData, sourceDataSize, hash );
		m_cache.Insert( hash, newToken );

		// return shared buffer
		return newToken.m_data;
	}
}

SharedDataBufferData* SharedDataBufferCache::Copy( SharedDataBufferData* data )
{
	if ( data != nullptr )
	{
		TCacheScopeLock lock( m_lock );

		// Retrieve hash
		const Uint64 hash = data->GetHash();

		// Find token
		Token* token = m_cache.FindPtr( hash );
		RED_FATAL_ASSERT( token != nullptr, "Buffer not found in cache. Memory stomp ?" );

		// Add extra reference
		token->m_numRefs += 1;
	}

	return data;
}

void SharedDataBufferCache::Release( SharedDataBufferData* data )
{
	RED_FATAL_ASSERT( data != nullptr, "Trying to release empty data" );

	{
		TCacheScopeLock lock( m_lock );

		// Retrieve hash
		const Uint64 hash = data->GetHash();

		// Find token
		Token* token = m_cache.FindPtr( hash );
		RED_FATAL_ASSERT( token != nullptr, "Buffer not found in cache. Memory stomp ?" );

		// Release reference, if not used any more, clear
		token->m_numRefs -= 1;
		if ( 0 == token->m_numRefs )
		{
			m_cache.Erase( hash );
			delete token->m_data;
		}
	}
}

Uint64 SharedDataBufferCache::CalcDataHash( const void* sourceData, const Uint32 sourceSize )
{
	return Red::CalculateHash64( sourceData, sourceSize );
}

//------------------
