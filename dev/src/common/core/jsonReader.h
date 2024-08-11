/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "rapidjson/stringbuffer.h"
#include "rapidjson/reader.h"
#include "rapidjson/document.h"
#include "jsonDocument.h"

template<class encoding, class allocator>
class CJSONReader
{
public:
	typedef typename encoding::Ch CharType;
	typedef rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>* CJSONObjectRef;

	CJSONReader(): mHasParseError( false ){}

	Bool Read( const CharType* content )
	{
		m_document.GetContext().template Parse<rapidjson::kParseDefaultFlags>( content );
		
		mHasParseError = m_document.GetContext().HasParseError();
		return !mHasParseError;
	}


	Bool HasParseError() { return mHasParseError; }

	const CJSONDocument<encoding, allocator>& GetDocument() const { return m_document; }

private:
	CJSONDocument<encoding, allocator> m_document;
	Bool mHasParseError;
};

typedef CJSONReader< rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)> CJSONReaderUTF8;
typedef CJSONReader< rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)> CJSONReaderUTF16;