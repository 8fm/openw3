#ifndef CORE_STATIC_ARRAY_NEW_H
#define CORE_STATIC_ARRAY_NEW_H

#include "../redContainers/array.h"
#include "../redContainers/bufferAllocatorStatic.h"

/////////////////////////////////////////////////////////////////////
// TStaticArray
//	A static array with RTTI support

template< class ElementType, Red::System::Uint32 MaxSize >
class TStaticArray : public Red::Containers::ArrayBase< ElementType, Red::Containers::BufferAllocatorStatic< ElementType, MaxSize > >
{
public:
	TStaticArray()
		: BaseClass()
	{
	}

	explicit TStaticArray( Red::System::Uint32 elementCount )
		: BaseClass( elementCount )
	{
	}

	template< class ArrayAllocatorType > TStaticArray( const Red::Containers::ArrayBase< ElementType, ArrayAllocatorType >& other )
		: BaseClass( other )
	{
	}

	~TStaticArray()
	{
	}

private:
	typedef Red::Containers::ArrayBase< ElementType, Red::Containers::BufferAllocatorStatic< ElementType, MaxSize > > BaseClass;
};

#endif