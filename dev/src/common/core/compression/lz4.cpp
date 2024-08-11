/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "lz4.h"

// The libraries are the same for Release and Final
#if defined( RED_CONFIGURATION_FINAL ) || defined( RED_CONFIGURATION_RELEASEGAME )
#	define COMPRESSION_DIR	Release
#else
#	define COMPRESSION_DIR	PROJECT_CONFIGURATION
#endif

#pragma comment( lib, RED_EXPAND_AND_STRINGIFY( ../../../external/compression/lz4/msvc/bin/PROJECT_PLATFORM/COMPRESSION_DIR/lz4.RED_PLATFORM_LIBRARY_EXT ) )

#include "../../../../external/compression/lz4/lz4.h"

namespace Red { namespace Core {

namespace Compressor {

CLZ4::CLZ4()
:	m_buffer( nullptr )
,	m_size( 0 )
{

}

CLZ4::~CLZ4()  
{
	if( m_buffer )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_L4Z, m_buffer );
		m_buffer = nullptr;
	}
}

/* static */ size_t CLZ4::GetRequiredAllocSize( Uint32 dataSize )
{
	if ( dataSize > 0 && dataSize < LZ4_MAX_INPUT_SIZE )
	{
		return LZ4_compressBound( static_cast< Int32 >( dataSize ) );
	}

	return 0;
}

/* static */ size_t CLZ4::CompressToPreAllocatedBuffer( const void* data, Uint32 dataSize, void* preAllocatedBuffer, size_t preAllocatedSize ) 
{
	if ( data && dataSize > 0 && preAllocatedBuffer && dataSize < LZ4_MAX_INPUT_SIZE )
	{
		// Calculate the largest possible buffer required to compress data of this size
		size_t maxSpaceRequired = LZ4_compressBound( static_cast< Int32 >( dataSize ) );

		if ( preAllocatedSize >= maxSpaceRequired )
		{
			// Cast to L4Z api type
			const AnsiChar* inData = static_cast< const AnsiChar* >( data );

			// Allocate a buffer for L4Z's internal bookkeeping structure
			void* lz4Data = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_L4Z, LZ4_sizeofState() );

			// Compress!
			size_t compressedSize = static_cast< Uint32 >( LZ4_compress_withState( lz4Data, inData, static_cast< AnsiChar* >( preAllocatedBuffer ), static_cast< Int32 >( dataSize ) ) );

			// Free internal L4Z data that's no longer required
			RED_MEMORY_FREE( MemoryPool_Default, MC_L4Z, lz4Data );

			return compressedSize;
		}
	}

	return 0;
}

Bool CLZ4::Compress( const void* data, Uint32 size )  
{
	if( data && size > 0 && size && size < LZ4_MAX_INPUT_SIZE )
	{
		// Calculate the largest possible buffer required to compress data of this size
		size_t maxSpaceRequired = LZ4_compressBound( static_cast< Int32 >( size ) );

		// Allocate a buffer for writing compressed data to
		m_buffer = static_cast< AnsiChar* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_L4Z, maxSpaceRequired ) );
		
		// Cast to L4Z api type
		const AnsiChar* inData = static_cast< const AnsiChar* >( data );

		// Allocate a buffer for L4Z's internal bookkeeping structure
		void* lz4Data = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_L4Z, LZ4_sizeofState() );

		// Compress!
		m_size = static_cast< Uint32 >( LZ4_compress_withState( lz4Data, inData, m_buffer, static_cast< Int32 >( size ) ) );

		// Free internal L4Z data that's no longer required
		RED_MEMORY_FREE( MemoryPool_Default, MC_L4Z, lz4Data );

		return m_size > 0;
	}

	return false;
}

const void* CLZ4::GetResult() const  
{
	return m_buffer;
}

Uint32 CLZ4::GetResultSize() const  
{
	return m_size;
}

} // namespace Compressor {

namespace Decompressor {

CLZ4::CLZ4()
:	m_in( nullptr )
,	m_out( nullptr )
,	m_inSize( 0 )
,	m_outSize( 0 )
{

}

CLZ4::~CLZ4()  
{

}

EStatus CLZ4::Initialize( const void* in, void* out, Uint32 inSize, Uint32 outSize )  
{
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
#else

	RED_ASSERT( in, TXT( "Must specify a valid buffer of compressed data" ) );
	RED_ASSERT( inSize > 0, TXT( "Compressed data buffer must have a size greater than zero" ) );
	RED_ASSERT( out, TXT( "Must specify a valid buffer in which to decompress data" ) );
	RED_ASSERT( outSize > 0, TXT( "Must specify a buffer with a size greater than zero in which to decompress data" ) );

#endif // RED_USE_DECOMPRESSION_ERROR_HANDLING

	m_in		= static_cast< const AnsiChar* >( in );
	m_out		= static_cast< AnsiChar* >( out );
	m_inSize	= static_cast< Int32 >( inSize );
	m_outSize	= static_cast< Int32 >( outSize );

	return Status_Success;
}

EStatus CLZ4::Decompress()
{
	if( LZ4_decompress_fast( m_in, m_out, m_outSize ) == m_inSize )
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
