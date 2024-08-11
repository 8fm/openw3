/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "jsonBasic.h"

template<class encoding, class allocator>
class CJSONArray: public CJSONBasic<encoding, allocator>
{
public:
	
	template<class encoding_array, class allocator_array> friend class CJSONArrayRef;

	CJSONArray( ): CJSONBasic<encoding, allocator>(new rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>( (rapidjson::Type) JSON_Array ) ) {}

	void Reserve( const MemSize newCapacity )
	{ 
		CJSONBasic<encoding, allocator>::GetContext().Reserve( (rapidjson::SizeType)newCapacity, allocator::PoolAllocatorInstance::GetInstance() );
	}

	void PushBack( const CJSONBasic<encoding, allocator>& object )
	{
		CJSONBasic<encoding, allocator>::PushBackBasic( object );
	}

	void PopBack()
	{
		CJSONBasic<encoding, allocator>::GetContext().PopBack();		
	}

	const MemSize Size() const
	{
		return (MemSize)CJSONBasic<encoding, allocator>::GetContext().Size();		
	}

private:
	rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>* GetContextPtr() const { return &CJSONBasic<encoding, allocator>::GetContext(); }
};

template<class encoding, class allocator>
class CJSONArrayRef: public CJSONBasicRef<encoding, allocator>
{
public:

	CJSONArrayRef( const CJSONArray<encoding, allocator>& base ): CJSONBasicRef<encoding, allocator>( base.GetContextPtr() ){}
	CJSONArrayRef( const CJSONBasicRef<encoding, allocator>& base ): CJSONBasicRef<encoding, allocator>( base )
	{
		if( CJSONBasicRef<encoding, allocator>::GetType() != JSON_Array)
		{
			CJSONBasicRef<encoding, allocator>::m_contextRef = NULL;
		}
	}

	CJSONBasicRef<encoding, allocator> GetMemberAt( const MemSize index ) const
	{
		return CJSONBasicRef<encoding, allocator>::GetMemberAtBasic( index );
	}

	const MemSize Size() const
	{
		return (MemSize)CJSONBasicRef<encoding, allocator>::GetContext().Size();		
	}

};

typedef CJSONArray< rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)> CJSONArrayUTF8;
typedef CJSONArrayRef< rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)> CJSONArrayRefUTF8;

typedef CJSONArray< rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json) > CJSONArrayUTF16;
typedef CJSONArrayRef< rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json) > CJSONArrayRefUTF16;