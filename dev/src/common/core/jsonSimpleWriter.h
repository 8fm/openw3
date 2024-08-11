/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "jsonWriter.h"

template<class encoding, class allocator>
class CJSONSimpleWriter: public CJSONWriter< encoding, allocator >
{
public:
	typedef typename encoding::Ch CharType;

	CJSONSimpleWriter()
	{
		m_writer = new rapidjson::Writer< rapidjson::GenericStringBuffer< encoding, typename allocator::PoolAllocator >, encoding, typename allocator::PoolAllocator >(*CJSONWriter< encoding, allocator >::m_strbuf, &(allocator::PoolAllocatorInstance::GetInstance()) );
	}

	virtual ~CJSONSimpleWriter()
	{
		delete m_writer;
	}
	
	virtual void WriteNull()
	{
		m_writer->Null();
	}
	virtual void WriteBool( Bool b )
	{
		m_writer->Bool( b );
	}
	virtual void WriteInt32( Int32 i )
	{
		m_writer->Int( i );
	}
	virtual void WriteUint32( Uint32 u )
	{
		m_writer->Uint( u );
	}
	virtual void WriteInt64( Int64 i64 )
	{
		m_writer->Int64( i64 );
	}
	virtual void WriteUint64( Uint64 u64 )
	{
		m_writer->Uint64( u64 );
	}
	virtual void WriteDouble( Double d )
	{
		m_writer->Double( d );
	}
	virtual void WriteString( const CharType* str )
	{
		m_writer->String( str );
	}
	virtual void WriteString( const CharType* str, MemSize length )
	{
		m_writer->String( str, (rapidjson::SizeType)length );
	}

	virtual void WriteObject( const CJSONObjectRef<encoding, allocator>& objectToSerialize )
	{
		objectToSerialize.GetContext().Accept( *m_writer );
	}
	virtual void WriteDocument( const CJSONDocument<encoding, allocator>& documentToSerialize )
	{
		documentToSerialize.GetContext().Accept( *m_writer );
	}

	virtual void StartObject()
	{
		m_writer->StartObject();
	}
	virtual void EndObject()
	{
		m_writer->EndObject();
	}

	virtual void StartArray()
	{
		m_writer->StartArray();
	}
	virtual void EndArray()
	{
		m_writer->EndArray();
	}


private:	
	rapidjson::Writer< rapidjson::GenericStringBuffer< encoding, typename allocator::PoolAllocator >, encoding, typename allocator::PoolAllocator >* m_writer;

};

typedef CJSONSimpleWriter< rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)> CJSONSimpleWriterUTF8;
typedef CJSONSimpleWriter< rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)> CJSONSimpleWriterUTF16;