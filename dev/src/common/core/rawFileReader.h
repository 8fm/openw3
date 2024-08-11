/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redIO/redIOFile.h"
#include "fileSystemProfilerWrapper.h"

/************************************************************************/
/* IFile implementation that handles unbuffered file reads				*/
/************************************************************************/
class CRawFileReader : public IFile
{
public:
	virtual ~CRawFileReader()
	{
	}

	// Serialize data buffer of given size
	virtual void Serialize( void* buffer, size_t size )
	{
		RED_FATAL_ASSERT( size < UINT_MAX, "Read size is to big" );

		Uint32 numRead = 0;
		m_fileHandle.Read( buffer, (Uint32) size, numRead );
		RED_UNUSED( numRead );

		// TODO: handle errors
		/*if ( size != sizeRead )
		{
			SetError();
		}*/
	}

	// Get position in file stream
	virtual Uint64 GetOffset() const
	{
		return m_fileHandle.Tell();
	}

	// Get size of the file stream
	virtual Uint64 GetSize() const
	{
		return m_fileHandle.GetFileSize();
	}

	// Seek to file position
	virtual void Seek( Int64 offset )
	{
		m_fileHandle.Seek( offset, Red::IO::eSeekOrigin_Set );

		// TODO: handle errors
		/*if ( size != sizeRead )
		{
			SetError();
		}*/
	}

	// Create file reader
	static CRawFileReader* Create( const Char* absoluteFilePath )
	{
		CRawFileReader* ret = new CRawFileReader();
		if ( ret->m_fileHandle.Open( absoluteFilePath, Red::IO::eOpenFlag_Read ) )
		{
			ret->m_filePath = absoluteFilePath;
			return ret;
		}
		else
		{
			delete ret;
			return nullptr;
		}
	}

	// For debug purposes only
	virtual const Char *GetFileNameForDebug() const
	{
		return m_filePath.AsChar();
	}

private:
	Red::IO::CNativeFileHandle	m_fileHandle;
	String						m_filePath;

	// Protected default constructor for mocking
	CRawFileReader()
		: IFile( FF_Reader | FF_FileBased )
	{
	}
};