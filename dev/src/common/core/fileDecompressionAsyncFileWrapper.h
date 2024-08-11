/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "asyncFileAccess.h"

/// IAsyncFile implementation that is using the decompression task to read and decompress the data
class CAsyncFile_DecompressionTaskWrapper : public IAsyncFile
{
public:
	CAsyncFile_DecompressionTaskWrapper( class IFileDecompressionTask* task, const String& debugFileName );

	// IAsyncFile interface
	virtual const Char* GetFileNameForDebug() const override;
	virtual const EResult GetReader( IFile*& outReader) const override;

private:
	virtual ~CAsyncFile_DecompressionTaskWrapper();

	class IFileDecompressionTask*		m_decompression;
	String								m_debugFileName;
};

/// IFile wrapper for decompressed data
class CFile_DecompressionTaskWrapper : public IFile, public IFileDirectMemoryAccess
{
public:
	CFile_DecompressionTaskWrapper( class IFileDecompressionTask* task, void* data, const Uint32 size, const String& debugFileName );
	virtual ~CFile_DecompressionTaskWrapper();

	// Serialize data buffer of given size
	virtual void Serialize( void* buffer, size_t size ) override;
	virtual Uint64 GetOffset() const override;
	virtual Uint64 GetSize() const override;
	virtual void Seek( Int64 offset ) override;
	virtual class IFileDirectMemoryAccess* QueryDirectMemoryAccess() override;
	virtual const Char *GetFileNameForDebug() const { return m_debugFileName.AsChar(); }

	// IFileDirectMemoryAccess interface
	virtual Uint8* GetBufferBase() const { return (Uint8*)m_data; }
	virtual Uint32 GetBufferSize() const { return m_localFileSize; }

private:
	void*		m_data;
	Uint32		m_localReadOffset;		// Local offset into the buffer
	Uint32		m_localFileSize;
	String		m_debugFileName;

	// memory owner
	class IFileDecompressionTask*	m_task;
};