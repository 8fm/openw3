/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_LZ4HC_COMPRESSION_H_
#define _RED_LZ4HC_COMPRESSION_H_

#include "compression.h"
#include "lz4.h"

namespace Red
{
	namespace Core
	{
		namespace Compressor
		{
			class CLZ4HC : public Base
			{
			public:
				CLZ4HC();
				virtual ~CLZ4HC() override final;

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
			// The same methods of decompression are used by the HC variant
			typedef CLZ4 CLZ4HC;
		}
	}
}

#endif // _RED_LZ4HC_COMPRESSION_H_
