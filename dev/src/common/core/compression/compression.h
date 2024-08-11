/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_COMPRESSION_H_
#define _RED_COMPRESSION_H_

#define RED_USE_DECOMPRESSION_ERROR_HANDLING

namespace Red
{
	namespace Core
	{
		namespace Compressor
		{
			class Base
			{
			public:
				Base();
				virtual ~Base();

				virtual Bool Compress( const void* data, Uint32 size ) = 0;

				// Returns a pointer to an internal buffer that will be free'd upon destruction of this object
				virtual const void* GetResult() const = 0;

				// Returns the size of the compressed data stored in the internal buffer (actual buffer size may be larger)
				virtual Uint32 GetResultSize() const = 0;
			};
		}

		namespace Decompressor
		{
			enum EStatus
			{
				Status_Success = 0,

				Status_InPointerNull,
				Status_OutPointerNull,
				Status_InSizeZero,
				Status_OutSizeZero,
				Status_OutSizeTooSmall,

				Status_InvalidData,

				Status_InvalidVersion,

				Status_Uninitialized,

				Status_Max
			};

			class Base
			{
			public:

			public:
				Base();
				virtual ~Base();

				virtual EStatus Initialize( const void* in, void* out, Uint32 inSize, Uint32 outSize ) = 0;

				virtual EStatus Decompress() = 0;
			};
		}
	}
}

#endif // _RED_COMPRESSION_H_
