#pragma once
#ifndef __FILE_NATIVE_READER___H__
#define __FILE_NATIVE_READER___H__

#include "file.h"

/// IFile based reader
class CFileHandleReader : public IFile
{
public:
	CFileHandleReader( class CNativeFileReader* reader );
	virtual ~CFileHandleReader();

	// IFile interface
	virtual void Serialize( void* buffer, size_t size ) override;
	virtual Uint64 GetOffset() const override;
	virtual Uint64 GetSize() const override;
	virtual void Seek( Int64 offset ) override;

private:
	class CNativeFileReader*		m_reader;
};

/// IFile based reader with abstracted offset and size
class CFileHandleReaderEx : public IFile
{
public:
	CFileHandleReaderEx( class CNativeFileReader* reader, const Uint32 offset, const Uint32 size );
	virtual ~CFileHandleReaderEx();

	// IFile interface
	virtual void Serialize( void* buffer, size_t size ) override;
	virtual Uint64 GetOffset() const override;
	virtual Uint64 GetSize() const override;
	virtual void Seek( Int64 offset ) override;

private:
	class CNativeFileReader*		m_reader;
	Uint32							m_size;
	Uint32							m_offset;
};

#endif // __FILE_NATIVE_READER___H__