/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_CHAINEDZLIB_H_
#define _RED_CHAINEDZLIB_H_

#include "compression.h"


namespace Red
{
	namespace Core
	{
		namespace Compressor
		{
			class CChainedZLib : public Base
			{
			public:
				CChainedZLib();
				virtual ~CChainedZLib() override final;

				// Compress a single chunk. Do this multiple times on different data with the same CChainedZLib, and the chunks will
				// be chained together. Use GetResultSize() before Compress to get the offset in the compressed data of the new chunk.
				virtual Bool Compress( const void* data, Uint32 size ) override final;

				virtual const void* GetResult() const override final;
				virtual Uint32 GetResultSize() const override final;

			private:
				void* m_outBuffer;
				Uint32 m_outBufferSize;

				TDynArray< Uint32 > m_markerOffsets;
			};
		}

		namespace Decompressor
		{
			class CChainedZLib : public Base
			{
			public:
				CChainedZLib();
				virtual ~CChainedZLib() override final;

				// Decompress chained data. Call Initialize with a pointer to the start of a chunk in the compressed data (does not
				// need to be the absolute start, as long as it's the start of a chunk), and Decompress will decompress from that
				// chunk to the end. inSize should be the size of the provided data (from 'in' to the end of the compressed data, not
				// the absolute size of the full compressed data).
				// If inSize and outSize correspond exactly to a subrange of the available chunks, just those chunks will be
				// decompressed. This allows, for example, decompressing 2 consecutive chunks from the middle of the whole compressed
				// data, by pointing 'in' to the first wanted chunk, setting 'inSize' to the compressed size of the 2 chunks, and
				// 'outSize' to the uncompressed size. If there's a mismatch in either size, then an error will be reported.
				virtual EStatus Initialize( const void* in, void* out, Uint32 inSize, Uint32 outSize ) override final;

				virtual EStatus Decompress() override final;

			private:
				Bool m_initialized;

				const void* m_inBuf;
				Uint32 m_inSize;

				void* m_outBuf;
				Uint32 m_outSize;
			};
		}
	}
}

#endif // _RED_CHAINEDZLIB_H_
