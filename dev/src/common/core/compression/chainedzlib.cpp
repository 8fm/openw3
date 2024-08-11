/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "chainedzlib.h"
#include "zlib.h"


// No reason to pad this. It's going at the start of a compressed stream so...
#pragma pack( push, 1 )
struct ChainedZLibMarker
{
	Uint32 m_compressedSize;
	Uint32 m_uncompressedSize;
	Uint8 m_chunksRemaining;
};
#pragma pack( pop )
static_assert( sizeof( ChainedZLibMarker ) == 9, "ChainedZLibMarker is unexpected size! It shouldn't be padded" );

static const Uint32 MARKER_SIZE = sizeof( ChainedZLibMarker );


namespace Red { namespace Core {

	namespace Compressor
	{

		CChainedZLib::CChainedZLib()
			: m_outBuffer( nullptr )
			, m_outBufferSize( 0 )
		{
		}

		CChainedZLib::~CChainedZLib()
		{
			if ( m_outBuffer )
			{
				RED_MEMORY_FREE( MemoryPool_Default, MC_Zlib, m_outBuffer );
				m_outBuffer = nullptr;
			}
		}

		Bool CChainedZLib::Compress( const void* data, Uint32 size )
		{
			CZLib zlib;
			if ( !zlib.Compress( data, size ) )
			{
				return false;
			}

			// Update counters for all existing markers
			for ( auto& markerOffset : m_markerOffsets )
			{
				ChainedZLibMarker* marker = static_cast< ChainedZLibMarker* >( OffsetPtr( m_outBuffer, markerOffset ) );
				RED_FATAL_ASSERT( marker->m_chunksRemaining < 255, "ChainedZLib chunk count overflow! Might need larger than Uint8, but more likely something is wrong!" );
				++marker->m_chunksRemaining;
			}

			Uint32 newSize = m_outBufferSize + MARKER_SIZE + zlib.GetResultSize();
			m_outBuffer = RED_MEMORY_REALLOCATE_ALIGNED( MemoryPool_Default, m_outBuffer, MC_Zlib, newSize, 8 );

			// Marker is at the start of the new chunk.
			ChainedZLibMarker* newMarker = static_cast< ChainedZLibMarker* >( OffsetPtr( m_outBuffer, m_outBufferSize ) );
			newMarker->m_chunksRemaining = 0;
			newMarker->m_compressedSize = zlib.GetResultSize();
			newMarker->m_uncompressedSize = size;

			Red::System::MemoryCopy( OffsetPtr( newMarker, MARKER_SIZE ), zlib.GetResult(), zlib.GetResultSize() );

			m_markerOffsets.PushBack( m_outBufferSize );
			m_outBufferSize = newSize;

			return true;
		}

		const void* CChainedZLib::GetResult() const
		{
			return m_outBuffer;
		}

		Uint32 CChainedZLib::GetResultSize() const
		{
			return m_outBufferSize;
		}

	}

	namespace Decompressor
	{

		CChainedZLib::CChainedZLib()
			: m_initialized( false )
			, m_inBuf( nullptr )
			, m_inSize( 0 )
			, m_outBuf( nullptr )
			, m_outSize( 0 )
		{
		}

		CChainedZLib::~CChainedZLib()
		{
		}

		EStatus CChainedZLib::Initialize( const void* in, void* out, Uint32 inSize, Uint32 outSize )
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

			// Need at least space for a single chunk marker.
			if ( inSize < MARKER_SIZE )
			{
				return Status_InvalidData;
			}


			m_inBuf = in;
			m_inSize = inSize;
			m_outBuf = out;
			m_outSize = outSize;

			m_initialized = true;

			return Status_Success;
		}

		EStatus CChainedZLib::Decompress()
		{
			if ( !m_initialized )
			{
				return Status_Uninitialized;
			}

			CZLib zlib;

			const void* in = m_inBuf;
			Uint32 inSize = m_inSize;

			void* out = m_outBuf;
			Uint32 outSize = m_outSize;

			// Get the number of chunks that need to be read in.
			const ChainedZLibMarker* firstMarker = static_cast< const ChainedZLibMarker* >( in );
			const Uint32 numChunks = firstMarker->m_chunksRemaining + 1;

			for ( Uint32 i = 0; i < numChunks; ++i )
			{
				// Not enough space for even the marker.
				if ( inSize < MARKER_SIZE )
				{
					return Status_InvalidData;
				}

				const ChainedZLibMarker* marker = static_cast< const ChainedZLibMarker* >( in );

				// Not enough space in the input data for the chunk.
				if ( inSize < marker->m_compressedSize + MARKER_SIZE )
				{
					return Status_InvalidData;
				}

				// Not enough space in the output.
				if ( outSize < marker->m_uncompressedSize )
				{
					return Status_InvalidData;
				}

				EStatus status = zlib.Initialize( OffsetPtr( in, MARKER_SIZE ), out, marker->m_compressedSize, outSize );
				if ( status != Status_Success )
				{
					return status;
				}

				status = zlib.Decompress();
				if ( status != Status_Success )
				{
					return status;
				}

				in = OffsetPtr( in, MARKER_SIZE + marker->m_compressedSize );
				inSize -= MARKER_SIZE + marker->m_compressedSize;

				out = OffsetPtr( out, marker->m_uncompressedSize );
				outSize -= marker->m_uncompressedSize;

				// If we've completely filled the output buffer at this point, then we'll stop here. This way, we can decompress
				// only a specific range of chunks, without having to introduce a bunch of extra parameters to the general decompression
				// interface, when it's only going to be used for this one decompressor, which is already a special case thing that
				// is used a bit differently from others.
				// If we aren't exactly fitting, then we'll loop around and probably hit the InvalidData return above for output buffer
				// not big enough. This is what we want, because if the output isn't exactly the size we're expecting, then there is
				// probably some logic error happening.
				if ( outSize == 0 && inSize == 0 )
				{
					return Status_Success;
				}
			}

			RED_ASSERT( inSize == 0, TXT("Size mismatch. End of compressed chunks reached, with %u bytes left"), inSize );

			return Status_Success;
		}

	}

} }
