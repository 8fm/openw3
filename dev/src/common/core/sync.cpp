/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef USE_RED_THREADS

#include "sync.h"

CSyncLightswitch::CSyncLightswitch()
	: m_mutex( 1, INT_MAX )
	, m_count( 0 )
{
}

void CSyncLightswitch::Lock( CSemaphore& semaphore )
{
	m_mutex.WaitForFinish();

		m_count += 1;
		if ( m_count == 1 )
		{
			semaphore.WaitForFinish();
		}

	m_mutex.Release(1);
}

void CSyncLightswitch::Unlock( CSemaphore& semaphore )
{
	m_mutex.WaitForFinish();

		m_count -= 1;
		if ( m_count == 0 )
		{
			semaphore.Release(1);
		}

	m_mutex.Release(1);
}

CSyncReaderWriter::CSyncReaderWriter()
	: m_mutex( 1, INT_MAX )
	, m_noReaders( 1, INT_MAX )
	, m_noWriters( 1, INT_MAX )
{
}

void CSyncReaderWriter::BeginRead()
{
	m_noReaders.WaitForFinish();
	m_readSwitch.Lock( m_noWriters );
	m_noReaders.Release( 1 );
}

void CSyncReaderWriter::EndRead()
{
	m_readSwitch.Unlock( m_noWriters );
}

void CSyncReaderWriter::BeginWrite()
{
	m_writeSwitch.Lock( m_noReaders );
	m_noWriters.WaitForFinish();
}

void CSyncReaderWriter::EndWrite()
{
	m_noWriters.Release( 1 );
	m_writeSwitch.Unlock( m_noReaders );
}

#endif // ! USE_RED_THREADS