/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "doboz.h"

// The libraries are the same for Release and Final
#if defined( RED_CONFIGURATION_FINAL ) || defined( RED_CONFIGURATION_RELEASEGAME )
#	define COMPRESSION_DIR	Release
#else
#	define COMPRESSION_DIR	PROJECT_CONFIGURATION
#endif

#pragma comment( lib, RED_EXPAND_AND_STRINGIFY( ../../../external/compression/doboz/bin/PROJECT_PLATFORM/COMPRESSION_DIR/doboz.RED_PLATFORM_LIBRARY_EXT ) )

#include "../../../../external/compression/doboz/Source/Doboz/Compressor.h"
#include "../../../../external/compression/doboz/Source/Doboz/Decompressor.h"

namespace Red { namespace Core {

namespace Compressor {

CDoboz::CDoboz()
:	m_buffer( nullptr )
,	m_size( 0 )
{

}

CDoboz::~CDoboz()
{
	if( m_buffer )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_Doboz, m_buffer );
		m_buffer = nullptr;
	}
}

Bool CDoboz::Compress( const void* data, Uint32 size )  
{
	if( data && size > 0 )
	{
		doboz::Compressor compressor;

		// Calculate the largest possible buffer required to compress data of this size
		Uint32 maxSpaceRequired = static_cast< Uint32 >( compressor.getMaxCompressedSize( size ) );

		// Allocate a buffer for writing compressed data to
		m_buffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Doboz, maxSpaceRequired );

		size_t compressedSize = 0;
		doboz::Result result = compressor.compress( data, size, m_buffer, maxSpaceRequired, compressedSize );

		m_size = static_cast< Uint32 >( compressedSize );

		if( result == doboz::RESULT_OK )
		{
			return true;
		}
	}

	return false;
}

const void* CDoboz::GetResult() const  
{
	return m_buffer;
}

Uint32 CDoboz::GetResultSize() const  
{
	return m_size;
}

} // namespace Compressor {

namespace Decompressor {


CDoboz::CDoboz()
:	m_in( nullptr )
,	m_out( nullptr )
,	m_inSize( 0 )
,	m_outSize( 0 )
{

}

CDoboz::~CDoboz()  
{

}

EStatus CDoboz::Initialize( const void* in, void* out, Uint32 inSize, Uint32 outSize )  
{
#ifdef RED_USE_DECOMPRESSION_ERROR_HANDLING
	doboz::Decompressor decompressor;

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
		doboz::CompressionInfo info;
		doboz::Result infoResult = decompressor.getCompressionInfo( in, inSize, info );

		if( infoResult == doboz::RESULT_ERROR_CORRUPTED_DATA )
		{
			return Status_InvalidData;
		}
		else if( infoResult == doboz::RESULT_ERROR_UNSUPPORTED_VERSION )
		{
			return Status_InvalidVersion;
		}
		else
		{
			if( info.uncompressedSize < outSize )
			{
				return Status_OutSizeTooSmall;
			}
		}
	}
#else

	RED_ASSERT( in, TXT( "Must specify a valid buffer of compressed data" ) );
	RED_ASSERT( inSize > 0, TXT( "Compressed data buffer must have a size greater than zero" ) );

	RED_ASSERT( out, TXT( "Must specify a valid buffer in which to decompress data" ) );

#ifdef RED_ASSERTS_ENABLED
	doboz::Decompressor decompressor;
	doboz::CompressionInfo info;
	doboz::Result infoResult = decompressor.getCompressionInfo( in, inSize, info );

	RED_ASSERT( infoResult == doboz::RESULT_OK, TXT( "Invalid compressed data buffer" ) );
	RED_ASSERT( outSize >= info.uncompressedSize, TXT( "Output data buffer is currently %u -> must be greater than or equal to %" ) RED_PRIWu64, outSize, info.uncompressedSize );
#endif // RED_ASSERTS_ENABLED

#endif // RED_USE_DECOMPRESSION_ERROR_HANDLING

	m_in		= in;
	m_out		= out;
	m_inSize	= inSize;
	m_outSize	= outSize;

	return Status_Success;
}

EStatus CDoboz::Decompress()  
{
	doboz::Decompressor decompressor;

	doboz::Result decompressionResult = decompressor.decompress( m_in, m_inSize, m_out, m_outSize );

	if( decompressionResult == doboz::RESULT_OK )
	{
		return Status_Success;
	}
	else if( decompressionResult == doboz::RESULT_ERROR_BUFFER_TOO_SMALL )
	{
		return Status_OutSizeTooSmall;
	}
	else if( decompressionResult == doboz::RESULT_ERROR_CORRUPTED_DATA )
	{
		return Status_InvalidData;
	}
	else if( decompressionResult == doboz::RESULT_ERROR_UNSUPPORTED_VERSION )
	{
		return Status_InvalidVersion;
	}

	return Status_InvalidData;
}

} // namespace Decompressor {

} } // namespace Red { namespace Core {
