/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "jsonObject.h"
#include "jsonDocument.h"

template<class encoding, class allocator>
class CJSONWriter
{
protected:
	CJSONWriter()
	{
		m_strbuf = new rapidjson::GenericStringBuffer< encoding, typename allocator::PoolAllocator >( &(allocator::PoolAllocatorInstance::GetInstance()) );
	}

	virtual ~CJSONWriter()
	{
		delete m_strbuf;
	}

public:
	typedef typename encoding::Ch CharType;

	void Clear() { m_strbuf->Clear(); }

	virtual void WriteNull() = 0;
	virtual void WriteBool( Bool b ) = 0;
	virtual void WriteInt32( Int32 i ) = 0;
	virtual void WriteUint32( Uint32 u ) = 0;
	virtual void WriteInt64( Int64 i64 ) = 0;
	virtual void WriteUint64( Uint64 u64 ) = 0;
	virtual void WriteDouble( Double d ) = 0;
	virtual void WriteString( const CharType* str ) = 0;
	virtual void WriteString( const CharType* str, MemSize length ) = 0;

	virtual void WriteObject( const CJSONObjectRef<encoding, allocator>& objectToSerialize ) = 0;
	virtual void WriteDocument( const CJSONDocument<encoding, allocator>& documentToSerialize ) = 0;

	virtual void StartObject() = 0;
	virtual void EndObject() = 0;

	virtual void StartArray() = 0;
	virtual void EndArray() = 0;
	

	const CharType* GetContent() 
	{ 
		return m_strbuf->GetString();
	}

protected:
	rapidjson::GenericStringBuffer< encoding, typename allocator::PoolAllocator >* m_strbuf;	
};
