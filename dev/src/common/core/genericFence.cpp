/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "genericFence.h"

//----

CGenericFence::CGenericFence( const AnsiChar* debugName )
	: m_debugName( debugName )
	, m_debugCreationTime( EngineTime::GetNow() )
	, m_refCount( 1 )
	, m_flags( 0 )
{
}

CGenericFence::~CGenericFence()
{
}

void CGenericFence::AddRef()
{
	Red::Threads::AtomicOps::Increment32( &m_refCount );
}

void CGenericFence::Release()
{
	if ( 0 == Red::Threads::AtomicOps::Decrement32( &m_refCount ) )
	{
		delete this;
	}
}

void CGenericFence::Signal( const Uint32 mask )
{
	Red::Threads::AtomicOps::Or32( &m_flags, mask );
}

const Uint32 CGenericFence::Test( const Uint32 mask ) const
{
	return (mask & m_flags);
}

const Double CGenericFence::GetDebugTimeSinceStart() const
{
	return ( EngineTime::GetNow() - m_debugCreationTime );
}

const AnsiChar* CGenericFence::GetDebugName() const
{
	return m_debugName;
}

//----

CGenericCountedFence::CGenericCountedFence( const AnsiChar* debugName, const Uint32 expectedCount )
	: m_debugName( debugName )
	, m_debugCreationTime( EngineTime::GetNow() )
	, m_refCount( 1 )
	, m_counter( expectedCount )
	, m_flag( false )
{
}

void CGenericCountedFence::AddRef()
{
	Red::Threads::AtomicOps::Increment32( &m_refCount );
}

void CGenericCountedFence::Release()
{
	if ( 0 == Red::Threads::AtomicOps::Decrement32( &m_refCount ) )
	{
		delete this;
	}
}

void CGenericCountedFence::Signal()
{
	const Int32 newCounter = Red::Threads::AtomicOps::Decrement32( &m_counter );
	if ( newCounter < 0 )
	{
		ERR_CORE( TXT("Counted fence '%ls' was overflown, current value = %d"), ANSI_TO_UNICODE( m_debugName ), newCounter );
	}
	else if ( newCounter == 0 )
	{
		m_debugSignalTime = EngineTime::GetNow();
		m_flag = true; // volatile any way
	}
}

const Bool CGenericCountedFence::Test() const
{
	return m_flag;
}

const Double CGenericCountedFence::GetDebugTimeSinceStart() const
{
	return ( EngineTime::GetNow() - m_debugCreationTime );
}

const Double CGenericCountedFence::GetDebugTimeWaiting() const
{
	return ( m_debugSignalTime - m_debugCreationTime );
}

const AnsiChar* CGenericCountedFence::GetDebugName() const
{
	return m_debugName;
}

CGenericCountedFence::~CGenericCountedFence()
{
}

//----

