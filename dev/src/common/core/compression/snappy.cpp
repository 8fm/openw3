/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "snappy.h"

// The libraries are the same for Release and Final
#if defined( RED_CONFIGURATION_FINAL ) || defined( RED_CONFIGURATION_RELEASEGAME )
#	define COMPRESSION_DIR	Release
#else
#	define COMPRESSION_DIR	PROJECT_CONFIGURATION
#endif

#pragma comment( lib, RED_EXPAND_AND_STRINGIFY( ../../../external/compression/snappy/msvc/bin/PROJECT_PLATFORM/COMPRESSION_DIR/snappy.RED_PLATFORM_LIBRARY_EXT ) )

namespace Red { namespace Core {

namespace Compressor {

CSnappy::CSnappy()
:	m_buffer( nullptr )
,	m_size( 0 )
{

}

CSnappy::~CSnappy()
{
	if( m_buffer )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_Snappy, m_buffer );
		m_buffer = nullptr;
	}
}

Bool CSnappy::Compress( const void* data, Uint32 size )
{
	if( data && size > 0 )
	{
		// Calculate the largest possible buffer required to compress data of this size
		size_t maxSpaceRequired = snappy::MaxCompressedLength( size );

		// Allocate a buffer for writing compressed data to
		m_buffer = static_cast< AnsiChar* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Snappy, maxSpaceRequired ) );

		size_t compressedSize;
		const AnsiChar* uncompressedData = static_cast< const AnsiChar* >( data );
		snappy::RawCompress( uncompressedData, size, m_buffer, &compressedSize );

		m_size = static_cast< Uint32 >( compressedSize );

		return true;
	}

	return false;
}

const void* CSnappy::GetResult() const
{
	return m_buffer;
}

Uint32 CSnappy::GetResultSize() const
{
	return m_size;
}

} // namespace Compressor {

namespace Decompressor {

CSnappy::CSnappy()
:	m_in( nullptr )
,	m_out( nullptr )
{

}

CSnappy::~CSnappy()
{

}

EStatus CSnappy::Initialize( const void* in, void* out, Uint32 inSize, Uint32 outSize )  
{
	//////////////////////////////////////////////////////////////////////////
	// Cast to types used by snappy
	m_in		= static_cast< const AnsiChar* >( in );
	m_out		= static_cast< AnsiChar* >( out );
	m_inSize	= inSize;

	//////////////////////////////////////////////////////////////////////////
	// Error Checking
#ifdef RED_USE_DECOMPRESSION_ERROR_HANDLING
	if( !in )
	{
		return Status_InPointerNull;
	}
	else if( !out )
	{
		return Status_OutPointerNull;
	}
	else if( inSize == 0 )
	{
		return Status_InSizeZero;
	}
	else if( outSize == 0 )
	{
		return Status_OutSizeZero;
	}
	else
	{
		if( !snappy::IsValidCompressedBuffer( m_in, inSize ) )
		{
			return Status_InvalidData;
		}
		else
		{
			size_t calculatedUncompressedSize = 0;

			if( snappy::GetUncompressedLength( m_in, inSize, &calculatedUncompressedSize ) )
			{
				if( outSize < calculatedUncompressedSize )
				{
					return Status_OutSizeTooSmall;
				}
			}
			else
			{
				return Status_InvalidData;
			}
		}
	}
#else

	RED_ASSERT( in, TXT( "Must specify a valid buffer of compressed data" ) );
	RED_ASSERT( inSize > 0, TXT( "Compressed data buffer must have a size greater than zero" ) );
	RED_ASSERT( IsValidCompressedBuffer( in, inSize ), TXT( "Compressed data buffer is invalid" ) );

	RED_ASSERT( out, TXT( "Must specify a valid buffer in which to decompress data" ) );

#ifdef RED_ASSERTS_ENABLED
	size_t calculatedUncompressedSize;
	RED_ASSERT( GetUncompressedLength( in, calculatedUncompressedSize ), TXT( "Invalid compressed data buffer" ) );
	RED_ASSERT( outSize >= calculatedUncompressedSize, TXT( "Compressed data buffer must have a size greater than or equal to %" ) RED_PRIWsize_t, calculatedUncompressedSize );
#endif // RED_ASSERTS_ENABLED

#endif // RED_USE_DECOMPRESSION_ERROR_HANDLING

	return Status_Success;
}

EStatus CSnappy::Decompress()
{
	//////////////////////////////////////////////////////////////////////////
	// Actual code starts here
	if( snappy::RawUncompress( m_in, m_inSize, m_out ) )
	{
		return Status_Success;
	}
	else
	{
		return Status_InvalidData;
	}
}

} // namespace Decompressor {

} } // namespace Red { namespace Core {
