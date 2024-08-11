/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "fileHandleCache.h"
#include "fileHandleReader.h"

//---

CFileHandleReader::CFileHandleReader( class CNativeFileReader* reader )
	: IFile( FF_Reader | FF_FileBased )
	, m_reader( reader )
{
}

CFileHandleReader::~CFileHandleReader()
{
	delete m_reader;
}

void CFileHandleReader::Serialize( void* buffer, size_t size )
{
	Uint32 numRead = 0;
	m_reader->Read( buffer, (Uint32) size, numRead );
	RED_ASSERT( numRead == size, TXT("IO error: read %d, expected %d"), size, numRead );
}

Uint64 CFileHandleReader::GetOffset() const
{
	return m_reader->Tell();
}

Uint64 CFileHandleReader::GetSize() const
{
	const Uint32 pos = m_reader->Tell();
	m_reader->Seek( 0, Red::IO::eSeekOrigin_End );
	const Uint32 size = m_reader->Tell();
	m_reader->Seek( pos, Red::IO::eSeekOrigin_Set );
	return size;
}

void CFileHandleReader::Seek( Int64 offset )
{
	RED_ASSERT( offset >= 0 && offset < UINT_MAX, TXT("IO error: invalid seek offset %d" ), offset );
	m_reader->Seek( (Uint32) offset, Red::IO::eSeekOrigin_Set );
}

//---

CFileHandleReaderEx::CFileHandleReaderEx( class CNativeFileReader* reader, const Uint32 offset, const Uint32 size )
	: IFile( FF_Reader | FF_FileBased )
	, m_reader( reader )
	, m_offset( offset )
	, m_size( size )
{
	m_reader->Seek( offset, Red::IO::eSeekOrigin_Set );
}

CFileHandleReaderEx::~CFileHandleReaderEx()
{
	delete m_reader;
}

void CFileHandleReaderEx::Serialize( void* buffer, size_t size )
{
	Uint32 numRead = 0;
	m_reader->Read( buffer, (Uint32)size, numRead );
	RED_ASSERT( numRead == size, TXT("IO error: read %d, expected %d"), size, numRead );
}

Uint64 CFileHandleReaderEx::GetOffset() const
{
	return (m_reader->Tell() - m_offset);
}

Uint64 CFileHandleReaderEx::GetSize() const
{
	return m_size;
}

void CFileHandleReaderEx::Seek( Int64 offset )
{
	RED_ASSERT( offset >= 0 && offset <= m_size, TXT("IO error: invalid seek offset %d" ), offset );
	const Uint32 realOffset = m_offset + (Uint32)offset;
	m_reader->Seek( realOffset, Red::IO::eSeekOrigin_Set );
}

//----