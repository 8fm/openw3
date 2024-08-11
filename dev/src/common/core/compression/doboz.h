/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_DOBOZ_COMPRESSION_H_
#define _RED_DOBOZ_COMPRESSION_H_

#include "compression.h"

namespace Red
{
	namespace Core
	{
		namespace Compressor
		{
			class CDoboz : public Base
			{
			public:
				CDoboz();
				virtual ~CDoboz() override final;

				virtual Bool Compress( const void* data, Uint32 size ) override final;

				virtual const void* GetResult() const override final;
				virtual Uint32 GetResultSize() const override final;

			private:
				void* m_buffer;
				Uint32 m_size;
			};
		}

		namespace Decompressor
		{
			class CDoboz : public Base
			{
			public:
				CDoboz();
				virtual ~CDoboz() override final;

				virtual EStatus Initialize( const void* in, void* out, Uint32 inSize, Uint32 outSize ) override final;

				virtual EStatus Decompress() override final;

			private:
				const void* m_in;
				void* m_out;
				size_t m_inSize;
				size_t m_outSize;
			};
		}
	}
}

#endif // _RED_DOBOZ_COMPRESSION_H_
