/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _CORE_UNIQUE_BUFFER_H_
#define _CORE_UNIQUE_BUFFER_H_

namespace Red
{
	class UniqueBuffer
	{
	public:

		UniqueBuffer();
		UniqueBuffer( void * buffer, Uint32 size, MemoryFramework::MemoryClass memClass );
		UniqueBuffer( UniqueBuffer && rvalue );
		~UniqueBuffer();

		void * Get() const;
		Uint32 GetSize() const;
		MemoryFramework::MemoryClass GetMemoryClass() const;

		void Reset();
		void Reset( void * buffer, Uint32 size, MemoryFramework::MemoryClass memClass );

		void * Release();

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
		Uint32 m_size;
		MemoryFramework::MemoryClass m_memoryClass;
	};

	UniqueBuffer CreateUniqueBuffer( Uint32 size, Uint32 alignment, MemoryFramework::MemoryClass memClass = MC_DataBlob );
}

#include "uniqueBuffer.inl"

#endif
