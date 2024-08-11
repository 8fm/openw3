/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/compression/compression.h"
#include "../core/compression/lz4.h"

class CMemoryFileWriter;
class CMemoryFileReader;

#define DEFAULT_LZ4_CHUNK_SZIE ( 1024 * 1024 ) /* 1 meg, do not change */

template< Uint32 CHUNK_SIZE >
struct SChunkedLZ4Compressor
{
	Uint8* m_in;
	Uint8* m_out;
	
	Uint32 m_inSizeLeft;
	Uint32 m_outSizeLeft;

	SChunkedLZ4Compressor( void* in, void* out, Uint32 inSize, Uint32 outSize )
		: m_in( ( Uint8* ) in )
		, m_out( ( Uint8* ) out )
		, m_inSizeLeft( inSize )
		, m_outSizeLeft( outSize )
	{
	}

	Uint32 Compress()
	{
		ASSERT( m_in && m_out && m_in != m_out && m_inSizeLeft && m_outSizeLeft );

		Uint32 total( 0 );

		// TODO: add buffer overrun checks
		while ( m_inSizeLeft )
		{
			const Uint32 chunkSize = Min( CHUNK_SIZE, m_inSizeLeft );
			Uint32& outSize = *( ( Uint32* ) m_out );
			outSize = ( Uint32 ) Red::Core::Compressor::CLZ4::CompressToPreAllocatedBuffer( m_in, chunkSize, m_out + 4, m_outSizeLeft - 4 );
			m_inSizeLeft -= chunkSize;
			m_outSizeLeft -= ( outSize + 4 );
			total += ( outSize + 4 );
			m_in += chunkSize;
			m_out += ( outSize + 4 );
		}

		return total;
	}

	void Decompress()
	{
		ASSERT( m_in && m_out && m_in != m_out && m_inSizeLeft && m_outSizeLeft );
		
		// TODO: add buffer overrun checks
		while ( m_inSizeLeft )
		{
			Uint32& inSize = *( ( Uint32* ) m_in );

			Red::Core::Decompressor::CLZ4 decompressor;
			decompressor.Initialize( m_in + 4, m_out, inSize, m_outSizeLeft );
			decompressor.Decompress();

			m_inSizeLeft -= ( inSize + 4 );
			m_outSizeLeft -= CHUNK_SIZE;

			m_in += ( inSize + 4 );
			m_out += CHUNK_SIZE; 
		}
	}
};

///////////////////////////////////////////////////////////////////////////////

/// Game save file writer
class CCompressedFileWriter : public IFileEx
{
private:
	TDynArray< Uint8 >	m_data;
	CMemoryFileWriter*	m_writer;

	String				m_fileName;

public:
	CCompressedFileWriter( const String& fileName );
	virtual ~CCompressedFileWriter();

public:
	// Serialize data buffer of given size
	virtual void Serialize( void* buffer, size_t size );

	// Get position in file stream
	virtual Uint64 GetOffset() const;

	// Get size of the file stream
	virtual Uint64 GetSize() const;

	// Seek to file position
	virtual void Seek( Int64 offset );

	// Save file to HDD
	virtual void Close();

	// Get data buffer
	virtual const void* GetBuffer() const;

	// Get the buffer allocation size
	virtual size_t GetBufferCapacity() const;
};

///////////////////////////////////////////////////////////////////////////////

/// Game save file reader
class CCompressedFileReader : public IFile
{
private:
	String				m_fileName;
	
	TDynArray< Uint8 >	m_data;
	CMemoryFileReader*	m_reader;

public:
	CCompressedFileReader( const String& fileName );
	virtual ~CCompressedFileReader();

public:
	// Serialize data buffer of given size
	virtual void Serialize( void* buffer, size_t size );

	// Get position in file stream
	virtual Uint64 GetOffset() const;

	// Get size of the file stream
	virtual Uint64 GetSize() const;

	// Seek to file position
	virtual void Seek( Int64 offset );
};

///////////////////////////////////////////////////////////////////////////////
