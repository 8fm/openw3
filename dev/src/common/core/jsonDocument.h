/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "jsonBasic.h"


template<class encoding, class allocator>
class CJSONDocument
{
public:
	typedef typename encoding::Ch CharType;

	template<class encoding_reader, class allocator_reader> friend class CJSONReader;
	template<class encoding_writer, class allocator_writer> friend class CJSONSimpleWriter;
	template<class encoding_writer, class allocator_writer> friend class CJSONPrettyWriter;

	CJSONBasicRef<encoding, allocator> GetMember( const CharType* name ) const
	{
		if( HasMember( name ) == true )
		{
			return CJSONBasicRef<encoding, allocator>( &GetContext()[name] );
		}
		return CJSONBasicRef<encoding, allocator>(NULL);
	}

	Bool HasMember( const CharType* name ) const
	{ 
		return GetContext().HasMember( name );
	}

private:

	CJSONDocument(): m_context( new rapidjson::GenericDocument< encoding, typename allocator::PoolAllocator >( &allocator::PoolAllocatorInstance::GetInstance() ) ){}
	
	rapidjson::GenericDocument< encoding, typename allocator::PoolAllocator >& GetContext() const
	{
		return *m_context;
	}

protected: 
	rapidjson::GenericDocument< encoding, typename allocator::PoolAllocator >* m_context;

};

typedef CJSONDocument< rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)> CJSONDocumentUTF8;
typedef CJSONDocument< rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)> CJSONDocumentUTF16;