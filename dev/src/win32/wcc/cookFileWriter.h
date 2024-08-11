/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

///----------------------

/// Output file used to save cooked stuff - in memory writing
class CCookerOutputFile : public IFile, public IFileDirectMemoryAccess
{
public:
	CCookerOutputFile( void* buffer, Uint32 bufferSize, const String& targetAbsolutePath );
	~CCookerOutputFile();

	// IFile interface
	virtual void Serialize( void* buffer, size_t size ) override;
	virtual Uint64 GetOffset() const override;
	virtual Uint64 GetSize() const override;
	virtual void Seek( Int64 offset ) override;
	virtual class IFileDirectMemoryAccess* QueryDirectMemoryAccess() override { return static_cast<IFileDirectMemoryAccess*>( this ); }

	// IFileDirectMemoryAccess interface
	virtual Uint8* GetBufferBase() const override;
	virtual Uint32 GetBufferSize() const override;

private:
	Uint8*			m_buffer;
	Uint32			m_bufferSize;

	Uint32			m_offset;
	Uint32			m_size;
	String			m_path;
};

///----------------------

/// Cooker file saving manager - created buffered files
class CCookerOutputFileManager
{
public:
	CCookerOutputFileManager();
	~CCookerOutputFileManager();

	// Flush all pending writes
	void Flush();

	// Alloc writer
	CCookerOutputFile* CreateWriter( const String& absoluteFilePath );

	// Schedule writing for buffer
	void ScheduleWriting( const void* data, const Uint32 size, const String& absoluteFilePath );

private:
	void*			m_buffer;
	Uint32			m_bufferSize;

	Uint32			m_maxPendingWriteSize;
	Uint32			m_pendingWriteSize;

	Red::Threads::CMutex	m_lock;

	// content was written, uncount the memory
	void FileWritten( Uint32 size );

	friend class CCookerOutputFileWritingJob;
};

typedef TSingleton< CCookerOutputFileManager > SCookerOutputFileManager;

///----------------------
