#include "types.h"
/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_CRC_H_
#define _RED_CRC_H_

#include "types.h"
#include "error.h"

namespace Red
{
	namespace System
	{
		class CRC32
		{
		public:
			CRC32();

			RED_INLINE Uint32 Calculate( const void* buffer, Uint32 bufferSize, Uint32 initialValue = 0 ) const
			{
				RED_FATAL_ASSERT( buffer != nullptr, "Cannot calculate CRC on null data" );
				return littleEndian( initialValue, buffer, bufferSize );
			}

		private:
			static void Initialize();
			Uint32 littleEndian( Uint32 c, const void* buf, Uint32 len ) const;

		private:
			static const Uint32 INNER_SIZE = 8;
			static const Uint32 OUTER_SIZE = 256;

			static Bool initialized;
			static Uint32 m_table[ INNER_SIZE ][ OUTER_SIZE ];
		};
	}
}

#endif // _RED_CRC_H_
