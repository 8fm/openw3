/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/file.h"

/// Fake FILE* based wrapper
/// We need a proper support for a read/write file

// Wrapper for reading
class CFakeIFileReaderWrapper : public IFile
{
protected:
	FILE*		m_file;

public:
	CFakeIFileReaderWrapper( FILE* file )
		: IFile( FF_Reader | FF_FileBased )
		, m_file( file )
	{};

	// Serialize data buffer of given size
	virtual void Serialize( void* buffer, size_t size )
	{
		fread( buffer, size, 1, m_file );
	}

	// Get position in file stream
	virtual Uint64 GetOffset() const
	{
#ifdef RED_PLATFORM_ORBIS
		RED_FATAL_ASSERT( false, "Not implemented on Orbis" );
		return (Uint64)-1;
#else
		return _ftelli64( m_file );
#endif
	}

	// Get size of the file stream
	virtual Uint64 GetSize() const
	{
#ifdef RED_PLATFORM_ORBIS
		Uint64 pos = ftell( m_file );
		fseek( m_file, 0, SEEK_END );
		unsigned int size = ftell( m_file );
		fseek( m_file, 0, SEEK_SET );
#else
		Uint64 pos = _ftelli64( m_file );
		_fseeki64( m_file, 0, SEEK_END );
		unsigned int size = ftell( m_file );
		_fseeki64( m_file, 0, SEEK_SET );
#endif
		RED_UNUSED( pos );
		return size;
	}

	// Seek to file position
	virtual void Seek( Int64 offset )
	{
#ifdef RED_PLATFORM_ORBIS
#else
		_fseeki64( m_file, offset, SEEK_SET );
#endif
	}
};

// Wrapper for writing
class CFakeIFileWriterWrapper : public IFile
{
protected:
	FILE*		m_file;

public:
	CFakeIFileWriterWrapper( FILE* file )
		: IFile( FF_Writer | FF_FileBased | ( GCNameAsNumberSerialization ? FF_HashNames : 0 ) )
		, m_file( file )
	{};

	// Serialize data buffer of given size
	virtual void Serialize( void* buffer, size_t size )
	{
		fwrite( buffer, size, 1, m_file );
		fflush( m_file );
	}

	// Get position in file stream
	virtual Uint64 GetOffset() const
	{
#ifdef RED_PLATFORM_ORBIS
		return ftell( m_file );
#else
		return _ftelli64( m_file );
#endif
	}

	// Get size of the file stream
	virtual Uint64 GetSize() const
	{
#ifdef RED_PLATFORM_ORBIS
		Uint64 pos = ftell( m_file );
		fseek( m_file, 0, SEEK_END );
		unsigned int size = ftell( m_file );
		fseek( m_file, 0, SEEK_SET );
#else
		Uint64 pos = _ftelli64( m_file );
		_fseeki64( m_file, 0, SEEK_END );
		unsigned int size = ftell( m_file );
		_fseeki64( m_file, 0, SEEK_SET );
#endif
		RED_UNUSED( pos );
		return size;
	}

	// Seek to file position
	virtual void Seek( Int64 offset )
	{
#ifdef RED_PLATFORM_ORBIS
#else
		_fseeki64( m_file, offset, SEEK_SET );
#endif
	}
};