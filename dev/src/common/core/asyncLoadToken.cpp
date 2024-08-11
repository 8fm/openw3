/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "asyncLoadToken.h"

#define FATAL_ASSERT(x) RED_FATAL_ASSERT((x), #x)

//////////////////////////////////////////////////////////////////////////
// CAsyncLoadToken
//////////////////////////////////////////////////////////////////////////
CAsyncLoadToken::CAsyncLoadToken( const String& absoluteFilePath, void* dest, Uint32 fileReadSize, Int64 fileOffset, EAsyncPriority asyncPriority /*= EAsyncPriority::AP_Normal*/ )
	: m_asyncLoadCallback( nullptr )
	, m_absoluteFilePath( absoluteFilePath )
	, m_buffer( dest )
	, m_numberOfBytesToRead( fileReadSize )
	, m_offset( fileOffset )
	, m_asyncPriority( asyncPriority )
	, m_refCount( 1 )
	, m_asyncResult( EAsyncResult::eAsyncResult_Pending )
	, m_event( false )
{
	RED_FATAL_ASSERT( dest, "No destination buffer" );
}

CAsyncLoadToken::CAsyncLoadToken( const String& absoluteFilePath, void* dest, Uint32 fileReadSize, Int64 fileOffset, IAsyncLoadCallback& asyncLoadCallback, EAsyncPriority asyncPriority /*= EAsyncPriority::AP_Normal*/ )
	: m_asyncLoadCallback( &asyncLoadCallback )
	, m_absoluteFilePath( absoluteFilePath )
	, m_buffer( dest )
	, m_numberOfBytesToRead( fileReadSize )
	, m_offset( fileOffset )
	, m_asyncPriority( asyncPriority )
	, m_refCount( 1 )
	, m_asyncResult( EAsyncResult::eAsyncResult_Pending )
	, m_event( false )
{
	RED_FATAL_ASSERT( dest, "No destination buffer" );
}

CAsyncLoadToken::~CAsyncLoadToken()
{
	FATAL_ASSERT( m_refCount.GetValue() == 0 );
}

void CAsyncLoadToken::AddRef()
{
	m_refCount.Increment();
}

Int32 CAsyncLoadToken::Release()
{
	const Int32 newRefCount = m_refCount.Decrement();
	FATAL_ASSERT( newRefCount >= 0 );
	if ( newRefCount == 0 )
	{
		delete this;
	}

	return newRefCount;
}

void CAsyncLoadToken::Wait()
{
//#if defined( RED_PLATFORM_DURANGO ) || defined( RED_PLATFORM_ORBIS )
	while ( ! m_event.GetValue() )
	{
		Red::Threads::SleepOnCurrentThread( 0 );
	}
//#endif
}

CAsyncLoadToken::EAsyncResult CAsyncLoadToken::GetAsyncResult() const
{
	if ( m_event.GetValue() )
	{
		return m_asyncResult.GetValue();
	}

	return EAsyncResult::eAsyncResult_Pending;
}

void CAsyncLoadToken::Signal( EAsyncResult result )
{
	m_asyncResult.SetValue( result );
	m_event.SetValue( true );

	if ( m_asyncLoadCallback )
	{
		m_asyncLoadCallback->OnAsyncLoad( *this, result );
	}
}

