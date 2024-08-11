/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "bundleFileReaderDecompression.h"

// Decompressors
#include "compression/zlib.h"
#include "compression/snappy.h"
#include "compression/doboz.h"
#include "compression/lz4.h"
#include "compression/lz4hc.h"
#include "compression/chainedzlib.h"
#include "compression/compression.h"

namespace BundleFileReaderDecompression
{
	template< class CompressionType >
	Bool DecompressWorker( const void* srcBuffer, Uint32 srcSize, void* destBuffer, Uint32 destSize )
	{
		CompressionType decompressor;

		// initialize stream
		const Red::Core::Decompressor::EStatus initResult = decompressor.Initialize( srcBuffer, destBuffer, srcSize, destSize );
		if ( initResult != Red::Core::Decompressor::Status_Success )
			return false;

		// decompress data
		const Red::Core::Decompressor::EStatus decompressResult = decompressor.Decompress();
		if ( decompressResult != Red::Core::Decompressor::Status_Success )
			return false;

		return true;
	}

	Bool DecompressFileBufferSynch( Red::Core::Bundle::ECompressionType compType, const void* srcBuffer, Uint32 srcSize, void* destBuffer, Uint32 destSize )
	{
		switch( compType )
		{
			case Red::Core::Bundle::CT_Zlib: 
				return DecompressWorker< Red::Core::Decompressor::CZLib >( srcBuffer, srcSize, destBuffer, destSize );

			case Red::Core::Bundle::CT_Snappy:
				return DecompressWorker< Red::Core::Decompressor::CSnappy >( srcBuffer, srcSize, destBuffer, destSize );

			case Red::Core::Bundle::CT_Doboz:
				return DecompressWorker< Red::Core::Decompressor::CDoboz >( srcBuffer, srcSize, destBuffer, destSize );

			case Red::Core::Bundle::CT_LZ4:
				return DecompressWorker< Red::Core::Decompressor::CLZ4 >( srcBuffer, srcSize, destBuffer, destSize );

			case Red::Core::Bundle::CT_LZ4HC:
				return DecompressWorker< Red::Core::Decompressor::CLZ4HC >( srcBuffer, srcSize, destBuffer, destSize );

			case Red::Core::Bundle::CT_ChainedZlib: 
				return DecompressWorker< Red::Core::Decompressor::CChainedZLib >( srcBuffer, srcSize, destBuffer, destSize );
		}

		return false;
	}

}

