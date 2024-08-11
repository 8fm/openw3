/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "file.h"
#include "dynarray.h"
#include "uniqueBuffer.h"

// This IFile interface is used to 'stream' compressed data to another IFile interface
// Each time data is written, it is buffered internally, then compressed and passed to
// the streamOut file when the buffer is full

struct CChunkedLZ4ChunkMetadata
{
	Uint32 m_compressedOffset;		// Position of data in 'raw file
	Uint32 m_compressedSize;		// Size of data in raw file
	Uint32 m_uncompressedSize;		// Size after decompression
};

class CChunkedLZ4FileWriter : public IFileEx
{
public:
	CChunkedLZ4FileWriter( Uint32 compressedChunkSize, Uint32 maximumChunks, IFile* streamOut );
	virtual ~CChunkedLZ4FileWriter();

	virtual void Serialize( void* buffer, size_t size );
	virtual Uint64 GetOffset() const;
	virtual Uint64 GetSize() const;
	virtual void Seek( Int64 offset );
	virtual void Close();
	virtual const void* GetBuffer() const;
	virtual size_t GetBufferCapacity() const;

private:
	void FlushToStream();
	IFile* m_streamOut;
	Red::UniqueBuffer m_chunkBuffer;
	Red::UniqueBuffer m_tempCompressedBuffer;
	Uint32 m_originalFileOffset;
	Uint32 m_bytesCached;
	Uint32 m_chunkStartOffset;		// start-offset of the chunk data (public offset, i.e. uncompressed)
	Uint32 m_fileSizeAfterClose;	// so we can get the file size after Close() has finished

	TDynArray< CChunkedLZ4ChunkMetadata, MC_Temporary > m_chunkHeaderData;
};

// CChunkedLZ4FileReader decompresses chunked file into a local buffer, then makes it available for reading
class CChunkedLZ4FileReader : public IFile
{
public:
	CChunkedLZ4FileReader( IFile* streamIn );
	virtual ~CChunkedLZ4FileReader();

	virtual void Serialize( void* buffer, size_t size );
	virtual Uint64 GetOffset() const;
	virtual Uint64 GetSize() const;
	virtual void Seek( Int64 offset );

private:
	struct ChunkMetadata
	{
		Uint32 m_compressedOffset;		// Position of data in 'raw file
		Uint32 m_compressedSize;
		Uint32 m_uncompressedOffset;	// Calculated at runtime
		Uint32 m_uncompressedSize;
	};

	typedef TDynArray< ChunkMetadata, MC_Temporary > ChunkHeaderMetadata;

	Bool LoadChunkMetadata();
	void PrepareForReading();
	Bool DecompressDataForReading( ChunkHeaderMetadata::iterator chunk );

	ChunkHeaderMetadata m_chunkHeaderData;
	IFile* m_rawStream;
	Uint32 m_firstChunkOffset;									// first chunk data address (for seeking)
	Uint32 m_decompressedFileSize;								// size of the file if it were decompressed
	Red::UniqueBuffer m_compressedBuffer;						// Keep a buffer around for loading compressed data

	// Store info about the current chunk we have in-memory
	TDynArray< Uint8, MC_Temporary > m_decompressedBuffer;		// Current chunk decompressed buffer
	ChunkHeaderMetadata::iterator m_currentChunk;				// Current chunk we have decompressed
	Uint32 m_localChunkOffset;									// Offset into decompressed buffer
};