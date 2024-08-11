/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_UNIQUE_BUFFER_H_
#define _RED_MEMORY_UNIQUE_BUFFER_H_

#include "operators.h"
#include <functional>

namespace red
{
	class UniqueBuffer
	{
	public:

		typedef std::function< void( void* ) > FreeCallback;

		UniqueBuffer();
		UniqueBuffer( void * buffer, Uint32 size, FreeCallback && callback );
		UniqueBuffer( UniqueBuffer && rvalue );
		~UniqueBuffer();

		void * Get() const;
		Uint32 GetSize() const;

		void * Release();

		void Reset();

		void Swap( UniqueBuffer & value );

		UniqueBuffer & operator=( UniqueBuffer && rvalue );

		struct BoolConversion{ int valid; };
		typedef int BoolConversion::*bool_operator;

		operator bool_operator () const;
		bool operator!() const;

	private:

		UniqueBuffer( const UniqueBuffer & );
		UniqueBuffer & operator=( const UniqueBuffer & );

		void * m_buffer;
		FreeCallback m_freeCallback;
		Uint32 m_size;
	};

	template< typename Pool >
	UniqueBuffer CreateUniqueBuffer( Uint32 size, Uint32 alignment );

	template< typename Proxy >
	UniqueBuffer CreateUniqueBuffer( Proxy & proxy, Uint32 size, Uint32 alignment );

	template< typename Pool >
	UniqueBuffer MakeUniqueBuffer( void * buffer, Uint32 size );

	template< typename Proxy >
	UniqueBuffer MakeUniqueBuffer( Proxy & proxy, void * buffer, Uint32 size );

}

#include "uniqueBuffer.hpp"

#endif
