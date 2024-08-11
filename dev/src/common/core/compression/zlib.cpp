/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

// The libraries are the same for Release and Final
#if defined( RED_CONFIGURATION_RELEASEGAME )
#	define COMPRESSION_DIR	Release
#elif defined( RED_CONFIGURATION_FINAL )
#	define COMPRESSION_DIR Final
#else
#	define COMPRESSION_DIR	PROJECT_CONFIGURATION
#endif

#pragma comment( lib, RED_EXPAND_AND_STRINGIFY( ../../../external/compression/zlib/lib/PROJECT_PLATFORM/COMPRESSION_DIR/zlib.RED_PLATFORM_LIBRARY_EXT ) )

#include "zlib.h"

namespace Red { namespace Core {

namespace Internal {

static voidpf Alloc( voidpf, uInt items, uInt size )
{
	void* retVal = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Zlib, size * items );

	if( !retVal )
	{
		return Z_NULL;
	}

	return retVal;
}

static void Free( voidpf, voidpf address )
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_Zlib, address );
}

} // namespace Internal {

namespace Compressor {

CZLib::CZLib()
:	m_outBuffer( nullptr )
,	m_outBufferSize( 0 )
{
	Red::System::MemoryZero( &m_zlib, sizeof( z_stream ) );
}

CZLib::~CZLib()
{
	if( m_outBuffer )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_Zlib, m_outBuffer );
		m_outBuffer = nullptr;
	}
}

Bool CZLib::Compress( const void* data, Uint32 size )
{
	return Compress( data, size, Z_DEFAULT_COMPRESSION );
}

Bool CZLib::Compress( const void* data, Uint32 size, Int32 ratio/*=-1*/ )
{
	Bool retVal			= true;

	m_outBuffer			= RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Default, MC_Zlib, size, 8 );
	m_outBufferSize		= size;
	
	m_zlib.next_in		= static_cast< z_const Bytef* >( data );
	m_zlib.avail_in		= size;
	
	m_zlib.next_out		= static_cast< Bytef* >( m_outBuffer );
	m_zlib.avail_out	= m_outBufferSize;

	m_zlib.zalloc		= &Internal::Alloc;
	m_zlib.zfree		= &Internal::Free;

	if( deflateInit( &m_zlib, (ratio != -1) ? ratio : Z_DEFAULT_COMPRESSION ) == Z_OK )
	{
		Bool finished = false;

		do 
		{
			Int32 result = deflate( &m_zlib, Z_FINISH );
			if( result == Z_STREAM_END )
			{
				finished = true;
			}
			else if( result == Z_BUF_ERROR )
			{
				// Ran out of space in the output buffer

				Uint32 newSize = m_outBufferSize * 2;
				m_outBuffer = RED_MEMORY_REALLOCATE_ALIGNED( MemoryPool_Default, m_outBuffer, MC_Zlib, newSize, 8 );

				m_zlib.next_out		= static_cast< Bytef* >( m_outBuffer ) + m_outBufferSize;
				m_zlib.avail_out	= m_outBufferSize;
				m_outBufferSize		= newSize;
			}
			else if( result != Z_OK )
			{
				// Something went wrong!
				retVal		= false;
				finished	= true;
			}

		} while( !finished );

		if( deflateEnd( &m_zlib ) != Z_OK )
		{
			retVal = false;
		}
	}
	else
	{
		// Failed to initialise
		retVal = false;
	}

	return retVal;
}

const void* CZLib::GetResult() const
{
	return m_outBuffer;
}

Uint32 CZLib::GetResultSize() const
{
	return m_zlib.total_out;
}

} // namespace Compressor {

namespace Decompressor {

CZLib::CZLib()
:	m_initialized( false )
{
	m_zlib.next_in		= nullptr;
	m_zlib.avail_in		= 0;

	m_zlib.next_out		= nullptr;
	m_zlib.avail_out	= 0;

	m_zlib.zalloc		= &Internal::Alloc;
	m_zlib.zfree		= &Internal::Free;
}

CZLib::~CZLib()  
{
	if( m_initialized )
	{
		RED_VERIFY( inflateEnd( &m_zlib ) == Z_OK, TXT( "Failed to clean up after zlib decompression" ) );
	}
}

EStatus CZLib::Initialize( const void* in, void* out, Uint32 inSize, Uint32 outSize )  
{
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
	RED_ASSERT( outSize > 0, TXT( "Uncompressed data buffer must have a size greater than zero" ) );
#endif

	m_zlib.next_in		= static_cast< z_const Bytef* >( in );
	m_zlib.avail_in		= inSize;

	m_outBuf = (Uint8*) out;
	m_outSize = outSize;

	if( !m_initialized )
	{
		Int32 initResult = inflateInit( &m_zlib );
		if( initResult == Z_OK )
		{
			m_initialized = true;
			return Status_Success;
		}
		else if( initResult == Z_VERSION_ERROR )
		{
			return Status_InvalidVersion;
		}
		else
		{
			return Status_InvalidData;
		}
	}
	else
	{
		Int32 resetResult = inflateReset( &m_zlib );

		if( resetResult == Z_OK )
		{
			return Status_Success;
		}
		else
		{
			return Status_InvalidData;
		}
	}
}

EStatus CZLib::Decompress()  
{
	EStatus retVal	= Status_Success;

	if( !m_initialized )
	{
		return Status_Uninitialized;
	}

	Uint8 localBuf[ 16*1024 ];

	const Uint32 bufferSize = ARRAY_COUNT_U32(localBuf);

	// decompress in 16KB blocks
	Uint32 writePos = 0;
	while ( 1 )
	{
		m_zlib.next_out = localBuf;
		Uint32 maxRead = Min< Uint32 >( m_outSize - writePos, bufferSize );
		m_zlib.avail_out = maxRead;

		auto result = inflate( &m_zlib, Z_NO_FLUSH );
		if ( result == Z_DATA_ERROR || result == Z_MEM_ERROR || result == Z_NEED_DICT )
		{
			return Status_InvalidData;
		}

		const Uint32 have = maxRead - m_zlib.avail_out;
		if ( have == 0 )
		{
			RED_FATAL_ASSERT( result == Z_STREAM_END, "Stream did not end when exepected" );
			break;
		}

		RED_FATAL_ASSERT( have + writePos <= m_outSize, "Zlib buffer overrun" );

		Red::MemoryCopy( m_outBuf + writePos, localBuf, have );

		writePos += have;
	}

	return retVal;
}

} // namespace Decompressor {

} } // namespace Red { namespace Core { 
