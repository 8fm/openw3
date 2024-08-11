/**
* Copyright © 2007 CDProjekt Red, Inc. All Rights Reserved.
*/
#pragma once

#ifndef USE_RED_THREADS

/// Light switch implementation for locking/unlocking single semaphore
class CSyncLightswitch
{
protected:
	CSemaphore			m_mutex;
	volatile Uint32		m_count;

public:
	CSyncLightswitch();
	void Lock( CSemaphore& semaphore );
	void Unlock( CSemaphore& semaphore );
};

/// Reader-Writer problem solution
class CSyncReaderWriter
{
protected:
	CSyncLightswitch		m_readSwitch;
	CSyncLightswitch		m_writeSwitch;
	CSemaphore				m_mutex;
	CSemaphore				m_noReaders;
	CSemaphore				m_noWriters;

public:
	CSyncReaderWriter();

	void BeginRead();
	void EndRead();

	void BeginWrite();
	void EndWrite();

public:
	//! Scoped class for reading
	class CScopedReader
	{
	private:
		CSyncReaderWriter*	m_readerWriter;

	public:
		RED_INLINE CScopedReader( CSyncReaderWriter* readerWriter )
			: m_readerWriter( readerWriter )
		{
			ASSERT( m_readerWriter );
			m_readerWriter->BeginRead();
		}

		RED_INLINE ~CScopedReader()
		{
			m_readerWriter->EndRead();
		}	
	};

public:
	//! Scoped class for writing
	class CScopedWriter
	{
	private:
		CSyncReaderWriter*	m_readerWriter;

	public:
		RED_INLINE CScopedWriter( CSyncReaderWriter* readerWriter )
			: m_readerWriter( readerWriter )
		{
			ASSERT( m_readerWriter );
			m_readerWriter->BeginWrite();
		}

		RED_INLINE ~CScopedWriter()
		{
			m_readerWriter->EndWrite();
		}	
	};
};

#endif // !USE_RED_THREADS