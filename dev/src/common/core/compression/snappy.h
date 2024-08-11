/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_SNAPPY_COMPRESSION_H_
#define _RED_SNAPPY_COMPRESSION_H_

#include "compression.h"

#include "../../../../external/compression/snappy/snappy.h"

namespace Red
{
	namespace Core
	{
		namespace Compressor
		{
			class CSnappy : public Base
			{
			public:
				CSnappy();
				virtual ~CSnappy() override final;

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
			class CSnappy : public Base
			{
			public:
				CSnappy();
				virtual ~CSnappy() override final;

				virtual EStatus Initialize( const void* in, void* out, Uint32 inSize, Uint32 outSize ) override final;

				virtual EStatus Decompress() override final;

			private:
				const AnsiChar* m_in;
				AnsiChar* m_out;
				size_t m_inSize;
			};
		}
	}
}

#endif // _RED_SNAPPY_COMPRESSION_H_
