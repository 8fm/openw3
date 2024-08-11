/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "cookFileWriter.h"

//--------

CCookerOutputFile::CCookerOutputFile( void* buffer, Uint32 bufferSize, const String& targetAbsolutePath )
	: IFile( FF_Buffered | FF_FileBased | FF_Writer | FF_NoBuffering )
	, m_buffer( (Uint8*) buffer )
	, m_bufferSize( bufferSize )
	, m_offset( 0 )
	, m_size( 0 )
	, m_path( targetAbsolutePath )
{
}

CCookerOutputFile::~CCookerOutputFile()
{
	// schedule for saving
	if ( m_size )
	{
		SCookerOutputFileManager::GetInstance().ScheduleWriting( m_buffer, m_size, m_path );
	}
}

void CCookerOutputFile::Serialize( void* buffer, size_t size )
{
	RED_FATAL_ASSERT( m_offset + (Uint32)size < m_bufferSize, "Internal cooking buffer is to small to save the cooked file." );
	Red::MemoryCopy( m_buffer + m_offset, buffer, size );
	m_offset += (Uint32) size;
	m_size = Max< Uint32 >( m_offset, m_size );
}

Uint64 CCookerOutputFile::GetOffset() const
{
	return m_offset;
}

Uint64 CCookerOutputFile::GetSize() const
{
	return m_size;
}

void CCookerOutputFile::Seek( Int64 offset )
{
	m_offset = (Uint32) offset;
}

Uint8* CCookerOutputFile::GetBufferBase() const
{
	return m_buffer;
}

Uint32 CCookerOutputFile::GetBufferSize() const
{
	return m_size;
}

//--------

class CCookerOutputFileWritingJob : public ILoadJob
{
public:
	CCookerOutputFileWritingJob( void* mem, const Uint32 size, const String& path )
		: ILoadJob( JP_Immediate, false )
		, m_memory( mem )
		, m_size( size )
		, m_path( path )
	{
	}

	~CCookerOutputFileWritingJob()
	{
		if ( m_memory )
		{
			RED_MEMORY_FREE( MemoryPool_Default, MC_CookedHeader, m_memory );
			m_memory = nullptr;
		}
	}

	virtual const Char* GetDebugName() const override { return TXT("CookerOutputFileWritingJob"); }

	virtual EJobResult Process() override 
	{
		// save content of the file to disk
		Uint32 numWritten = 0;
		IFile* file = GFileManager->CreateFileWriter( m_path, FOF_AbsolutePath );
		if ( file )
		{
			file->Serialize( m_memory, m_size );
			numWritten = (Uint32) file->GetOffset();
			delete file;
		}

		// check if data was written
		if ( numWritten != m_size )
		{
			ERR_WCC( TXT("!!! FATAL WRITING ERROR !!! Unable to save data to '%ls' (saved %d out of %d)"), m_path.AsChar(), numWritten, m_size );
		}

		// free memory
		if ( m_memory )
		{
			RED_MEMORY_FREE( MemoryPool_Default, MC_CookedHeader, m_memory );
			m_memory = nullptr;
		}

		// uncount
		SCookerOutputFileManager::GetInstance().FileWritten( m_size );

		// done
		return JR_Finished;
	};

private:
	String			m_path;
	void*			m_memory;
	Uint32			m_size;
};

//--------

CCookerOutputFileManager::CCookerOutputFileManager()
	: m_bufferSize( 100 * 1024 * 1024 )
{
	// allocate memory
	m_buffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_CookedHeader, m_bufferSize );

	// writing queue size
	m_maxPendingWriteSize = 100 * 1024 * 1024;
	m_pendingWriteSize = 0;
}

CCookerOutputFileManager::~CCookerOutputFileManager()
{
	if ( m_buffer )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_CookedHeader, m_buffer );
		m_buffer = nullptr;
	}
}

void CCookerOutputFileManager::Flush()
{
	SJobManager::GetInstance().FlushPendingJobs();
}

CCookerOutputFile* CCookerOutputFileManager::CreateWriter( const String& absoluteFilePath )
{
	// create the wrapper
	return new CCookerOutputFile( m_buffer, m_bufferSize, absoluteFilePath );
}

void CCookerOutputFileManager::FileWritten( Uint32 size )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
	RED_FATAL_ASSERT( size <= m_pendingWriteSize, "Invalid data in file saving queue" );
	m_pendingWriteSize -= size;
}

void CCookerOutputFileManager::ScheduleWriting( const void* data, const Uint32 size, const String& absoluteFilePath )
{
	void* tempMem = nullptr;

	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

		// to much, flush the queue
		if ( m_pendingWriteSize + size > m_maxPendingWriteSize )
		{
			// release the lock to allow the job queue to be flushed
			m_lock.Release();

			// flush the job queue
			SJobManager::GetInstance().FlushPendingJobs();

			// make sure we cleared the job queue
			RED_ASSERT( m_pendingWriteSize == 0, TXT("Job list not empty after flush, WTF?" ) );

			// reacquire the lock
			m_lock.Acquire();
		}

		// out of memory, write directly
		if ( m_pendingWriteSize + size > m_maxPendingWriteSize )
		{
			IFile* file = GFileManager->CreateFileWriter( absoluteFilePath, FOF_AbsolutePath, size );
			if ( file )
			{
				file->Serialize( (void*)data, size );
				delete file;
			}

			m_lock.Release();
			return;
		}

		// allocate writing memory
		tempMem = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_CookedHeader, size );
		Red::MemoryCopy( tempMem, data, size );

		// add writing task
		m_pendingWriteSize += size;
	}

	// create job
	CCookerOutputFileWritingJob* job = new CCookerOutputFileWritingJob( tempMem, size, absoluteFilePath );
	SJobManager::GetInstance().Issue( job );
}

//--------
