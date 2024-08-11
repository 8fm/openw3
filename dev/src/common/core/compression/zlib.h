/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_ZLIB_H_
#define _RED_ZLIB_H_

#include "compression.h"

#define ZLIB_CONST
#include "../../../../external/compression/zlib/zlib.h"

namespace Red
{
	namespace Core
	{
		namespace Compressor
		{
			class CZLib : public Base
			{
			public:
				CZLib();
				virtual ~CZLib() override final;

				virtual Bool Compress( const void* data, Uint32 size ) override final;

				Bool Compress( const void* data, Uint32 size, Int32 ratio );

				virtual const void* GetResult() const override final;
				virtual Uint32 GetResultSize() const override final;

			private:
				z_stream m_zlib;
				void* m_outBuffer;
				Uint32 m_outBufferSize;
			};
		}

		namespace Decompressor
		{
			class CZLib : public Base
			{
			public:
				CZLib();
				virtual ~CZLib() override final;

				virtual EStatus Initialize( const void* in, void* out, Uint32 inSize, Uint32 outSize ) override final;

				virtual EStatus Decompress() override final;

			private:
				z_stream m_zlib;
				Bool m_initialized;

				Uint8* m_outBuf;
				Uint32 m_outSize;
			};
		}
	}
}

#endif // _RED_ZLIB_H_
