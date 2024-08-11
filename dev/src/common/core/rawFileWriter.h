/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/************************************************************************/
/* IFile implementation that handles unbuffered file writes				*/
/************************************************************************/

class CRawFileWriter : public IFile
{
protected:
	CSystemFile	m_fileHandle;

public:
	CRawFileWriter( const CSystemFile& fileHandle )
		: IFile( FF_Writer | FF_FileBased | ( GCNameAsNumberSerialization ? FF_HashNames : 0 ) )
		, m_fileHandle( fileHandle )
	{
	}

	virtual ~CRawFileWriter()
	{
		m_fileHandle.Close( );
	}

	// Serialize data buffer of given size
	virtual void Serialize( void* buffer, size_t size )
	{
#ifndef RED_FINAL_BUILD
#ifdef RED_ARCH_X64
		extern Red::Threads::AtomicOps::TAtomic64 GSystemIONumBytesWritten;
		Red::Threads::AtomicOps::ExchangeAdd64( &GSystemIONumBytesWritten, size );
#endif
#endif

		const uintptr_t actualWrite = m_fileHandle.Write( buffer, size );
		if ( actualWrite != size )
		{
			GFileManager->HandleIOError( *this, FMT_LOG_MSG(TXT("CRawFileWriter write error: requested %llu bytes, sent %llu bytes"), (Uint64)size, (Uint64)actualWrite) );
		}
	}

	// Get position in file stream
	virtual Uint64 GetOffset() const
	{
		Uint64 offset;
		m_fileHandle.GetPointerCurrent( offset );
		return offset;
	}

	// Get size of the file stream
	virtual Uint64 GetSize() const
	{
		return m_fileHandle.GetSize( );
	}

	// Seek to file position
	virtual void Seek( Int64 offset )
	{
		if ( !m_fileHandle.SetPointerBegin( offset ) )
		{
			GFileManager->HandleIOError( *this, FMT_LOG_MSG( TXT("CRawFileWriter seek error: offset %lld in file of size %llu bytes"), (Int64)offset, (Uint64)GetSize() ) );
		}
	}

public:
	// Create file writer
	static CRawFileWriter* Create( const Char* absoluteFilePath, Bool append )
	{
		CSystemFile sysFile;
		if ( sysFile.CreateWriter( absoluteFilePath, append ) )
		{
#ifndef RED_FINAL_BUILD
#ifdef RED_ARCH_X64
			extern Red::Threads::AtomicOps::TAtomic64 GSystemIONumFilesOpened;
			Red::Threads::AtomicOps::Increment64( &GSystemIONumFilesOpened );
#endif		
#endif		
			return new CRawFileWriter( sysFile );
		}

		// Not found
		return NULL;
	}
};