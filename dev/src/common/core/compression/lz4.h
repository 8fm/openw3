/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_LZ4_COMPRESSION_H_
#define _RED_LZ4_COMPRESSION_H_

#include "compression.h"

namespace Red
{
	namespace Core
	{
		namespace Compressor
		{
			class CLZ4 : public Base
			{
			public:
				CLZ4();
				virtual ~CLZ4() override final;

				static size_t GetRequiredAllocSize( Uint32 dataSize );

				static size_t CompressToPreAllocatedBuffer( const void* data, Uint32 dataSize, void* preAllocatedBuffer, size_t preAllocatedSize );
				virtual Bool Compress( const void* data, Uint32 size ) override final;

				virtual const void* GetResult() const override final;
				virtual Uint32 GetResultSize() const override final;

			private:
				AnsiChar* m_buffer;
				Uint32 m_size;
			};
		}

		namespace Decompressor
		{
			class CLZ4 : public Base
			{
			public:
				CLZ4();
				virtual ~CLZ4() override final;

				virtual EStatus Initialize( const void* in, void* out, Uint32 inSize, Uint32 outSize ) override final;

				virtual EStatus Decompress() override final;

			private:
				const AnsiChar* m_in;
				AnsiChar* m_out;
				Int32 m_inSize;
				Int32 m_outSize;
			};
		}
	}
}

#endif // _RED_LZ4_COMPRESSION_H_
