#ifndef CORE_DYNAMIC_ARRAY_NEW_H
#define CORE_DYNAMIC_ARRAY_NEW_H

#include "../redContainers/array.h"
#include "../redContainers/bufferAllocatorDynamic.h"
#include <typeinfo>
#include "memory.h"

// Dynamic array buffer allocator
template< Red::MemoryFramework::MemoryClass MemClass, Red::MemoryFramework::PoolLabel MemPool >
class DynamicArrayAllocator
{
public:
	static void* Reallocate( void* ptr, Red::System::MemSize size )
	{
		return RED_MEMORY_REALLOCATE( MemPool, ptr, MemClass, size );
	}
};

// Dynamic array buffer policy
template< class ElementType, Red::MemoryFramework::MemoryClass MemClass, Red::MemoryFramework::PoolLabel MemPool >
class DynamicArrayBuffer : public Red::Containers::BufferAllocatorDynamic< ElementType, DynamicArrayAllocator< MemClass, MemPool > >
{

};

// Dynamic array class
template< class ElementType, 
	Red::MemoryFramework::MemoryClass MemClass = MC_DynArray,
	Red::MemoryFramework::PoolLabel MemPool = MemoryPool_Default >
class TDynArray : public Red::Containers::ArrayBase< ElementType, DynamicArrayBuffer< ElementType, MemClass, MemPool > >
{
public:
	TDynArray()
		: BaseClass()
	{
	}

	explicit TDynArray( Red::System::Uint32 elementCount )
		: BaseClass( elementCount )
	{
	}

	template< class ArrayAllocatorType > 
	TDynArray( const Red::Containers::ArrayBase< ElementType, ArrayAllocatorType >& other )
		: BaseClass()
	{
		PushBack( other );
	}

	~TDynArray()
	{
	}

	// Array Serialisation
	friend IFile& operator<<( IFile& file, TDynArray< ElementType, MemClass, MemPool > &ar )
	{
		ar.Serialize( file );
		return file;
	}

	// Serialize array data
	void Serialize( IFile& file )
	{
		if ( file.IsReader() )
		{
			// Read the number of elements in the array
			Uint32 size = 0;
			file << CCompressedNumSerializer( size );

			// Initialize array
			Resize( size );
		}
		else if ( file.IsWriter() )
		{
			// Write number of elements in the array
			Uint32 size = Size();
			file << CCompressedNumSerializer( size );
		}

		// Serialize elements
		Bool isPlainType = TPlainType< ElementType >::Value;
#ifdef RED_ENDIAN_SWAP_SUPPORT_DEPRECATED
		if ( isPlainType && !file.IsByteSwapping() )
#else
		if ( isPlainType )
#endif
		{
			// Serialize whole buffer
			if ( Size() > 0 )
			{
				file.Serialize( Data(), Size() * sizeof( ElementType ) );
			}
		}
		else
		{
			// Serialize each element
			for ( Red::System::Uint32 i=0; i<Size(); i++ )
			{
				file << TypedData()[i];
			}
		}
	}

	// Serialize as data bulk
	void SerializeBulk( IFile& file )
	{
		if ( file.IsReader())
		{
			// Read the number of elements in the array
			Uint32 size = 0;
			file << CCompressedNumSerializer( size );

			// Initialize array
			Resize( size );			
		}
		else if ( file.IsWriter() )
		{
			// Write number of elements in the array
			Uint32 size = Size();
			file << CCompressedNumSerializer( size );
		}

		// Serialize whole buffer
		file.Serialize( Data(), Size() * sizeof( ElementType ) );
	}

	// Resizes the internal buffer to exactly fit the data. Only applicable to dynamic arrays
	// Does not call destructors on destroyed elements since they are not valid
	RED_INLINE void Shrink()
	{
		if( Size() < Capacity() )
		{
			m_bufferPolicy.ResizeBuffer( Size() );
		}
	}

private:
	typedef Red::Containers::ArrayBase< ElementType, DynamicArrayBuffer< ElementType, MemClass, MemPool > > BaseClass;
};

#endif
