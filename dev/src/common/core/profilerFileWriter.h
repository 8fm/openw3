/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

///-----

/// Buffered and threaded file writer for profiling use
class CProfilerFileWriter
{
public:
	~CProfilerFileWriter();

	// Open profiling output at given file, can fail
	static CProfilerFileWriter* Open( const String& absoluteFilePath, const Uint32 bufferSize = 0 );

	// Allocate data in the write buffer, atomic, thread safe
	Uint8* AllocMessage( const Uint32 size );

	// Allocate data in the write buffer, atomic, thread safe
	template< typename T >
	T* AllocMessage()
	{
		return AllocMessage( sizeof(T) );
	}

	// Finish the message
	void FinishMessage();

	// Flush to file
	void Flush();

private:
	CProfilerFileWriter();

	// current write buffer (thread safe)
	volatile Uint32		m_writePos;
	Uint32				m_writeMax;
	Uint8* volatile		m_writeBuffer;

	// message count
	Red::Threads::CAtomic< Uint32 >		m_activeMessages;
	Uint32								m_pendingMessages;

	// default size of the buffer
	Uint32								m_bufferSize;

	// info about a buffer that is not yet flushed
	struct BufferInfo
	{
		Uint8*		m_data;
		Uint32		m_size;
	};

	// queue of not flushed buffers
	typedef TDynArray< BufferInfo >		TBuffersToFlush;
	TBuffersToFlush		m_flushList;

	// output PHYSICAL file
	CSystemFile			m_file;
	Bool				m_ioError;

	// internal lock
	Red::Threads::CMutex		m_fullLock;
	Red::Threads::CMutex		m_fastLock;
	Red::Threads::CMutex		m_writeLock;

	// allocate memory buffer
	static Uint8* AllocProfilingMemory( const Uint32 size );
	static void FreeProfilingMemory( void* memory );

	// save writing
	void SafeWrite( const void* data, const Uint32 size );
};

///-----

/// Helper class used to format the messages
template< typename T >
class CProfilerMessage
{
public:
	RED_FORCE_INLINE CProfilerMessage( CProfilerFileWriter* writer )
		: m_writer( writer )
	{
		m_data = (T*) m_writer->AllocMessage( sizeof(T) );
	}

	RED_FORCE_INLINE ~CProfilerMessage()
	{
		m_writer->FinishMessage();
	}

	RED_FORCE_INLINE T* operator->()
	{
		return m_data;
	}

private:
	CProfilerFileWriter*		m_writer;
	T*							m_data;
};

///-----
