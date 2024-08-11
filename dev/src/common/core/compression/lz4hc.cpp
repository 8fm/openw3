/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "lz4hc.h"

// The libraries are the same for Release and Final
#if defined( RED_CONFIGURATION_FINAL ) || defined( RED_CONFIGURATION_RELEASEGAME )
#	define COMPRESSION_DIR	Release
#else
#	define COMPRESSION_DIR	PROJECT_CONFIGURATION
#endif

#pragma comment( lib, RED_EXPAND_AND_STRINGIFY( ../../../external/compression/lz4/msvc/bin/PROJECT_PLATFORM/COMPRESSION_DIR/lz4.RED_PLATFORM_LIBRARY_EXT ) )

#include "../../../../external/compression/lz4/lz4.h"
#include "../../../../external/compression/lz4/lz4hc.h"

namespace Red { namespace Core {

namespace Compressor {

CLZ4HC::CLZ4HC()
:	m_buffer( nullptr )
,	m_size( 0 )
{

}

CLZ4HC::~CLZ4HC()  
{
	if( m_buffer )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_L4Z, m_buffer );
		m_buffer = nullptr;
	}
}

Bool CLZ4HC::Compress( const void* data, Uint32 size )  
{
	if( data && size > 0 && size && size < LZ4_MAX_INPUT_SIZE )
	{
		// Calculate the largest possible buffer required to compress data of this size
		Int32 maxSpaceRequired = LZ4_compressBound( static_cast< Int32 >( size ) );

		// Allocate a buffer for writing compressed data to
		m_buffer = static_cast< AnsiChar* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_L4Z, maxSpaceRequired ) );

		// Cast to L4Z api type
		const AnsiChar* inData = static_cast< const AnsiChar* >( data );

		// Allocate a buffer for L4Z's internal bookkeeping structure
		void* lz4Data = RED_ALLOCA( LZ4_sizeofStateHC() );

		// Compress!
		m_size = static_cast< Uint32 >( LZ4_compressHC_withStateHC( lz4Data, inData, m_buffer, static_cast< Int32 >( size ) ) );

		RED_FATAL_ASSERT( m_size != 0, "LZ4HC Failed to compress" );
		RED_FATAL_ASSERT( m_size <= static_cast< Uint32 >( maxSpaceRequired ), "LZ4HC provided an invalid upper bound: It specified that we needed at most a buffer of size %u, but wrote %u instead" );

		return m_size > 0;
	}

	return false;
}

const void* CLZ4HC::GetResult() const  
{
	return m_buffer;
}

Uint32 CLZ4HC::GetResultSize() const  
{
	return m_size;
}

} // namespace Compressor {

} } // namespace Red { namespace Core {
