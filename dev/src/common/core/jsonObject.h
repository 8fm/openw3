/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "jsonValue.h"

template<class encoding, class allocator>
class CJSONObject: public CJSONBasic<encoding, allocator>
{

public:
	typedef typename encoding::Ch CharType;

	template<class encoding_object, class allocator_object> friend class CJSONObjectRef;

	CJSONObject( ): CJSONBasic<encoding, allocator>(new rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>( (rapidjson::Type) JSON_Object ) ) {}

	//! Object
	void AddMember( const CharType* name, const CJSONBasic<encoding,allocator>& object )
	{	
		CJSONBasic<encoding,allocator>::AddMemberBasic( name, object );
	}

	//! Construct and add bool value.
	void AddMemberBool( const CharType* name, Bool b )
	{
		CJSONValue<encoding,allocator> valueObject( b );
		AddMember( name, valueObject );
	}

	//! Construct and add int value.
	void AddMemberInt32( const CharType* name, Int32 i )
	{
		CJSONValue<encoding,allocator> valueObject( i );
		AddMember( name, valueObject );
	}

	//! Construct and add unsigned value.
	void AddMemberUint32( const CharType* name, Uint32 u )
	{
		CJSONValue<encoding,allocator> valueObject( u );
		AddMember( name, valueObject );
	}

	//! Construct and add int64_t value.
	void AddMemberInt64( const CharType* name, Int64 i64 ) 
	{
		CJSONValue<encoding,allocator> valueObject( i64 );
		AddMember( name, valueObject );
	}

	//! Construct and add uint64_t value.
	void AddMemberUint64( const CharType* name, Uint64 u64 ) 
	{
		CJSONValue<encoding,allocator> valueObject( u64 );
		AddMember( name, valueObject );
	}

	//! Construct and add double value.
	void AddMemberDouble( const CharType* name, Double d )
	{
		CJSONValue<encoding,allocator> valueObject( d );
		AddMember( name, valueObject );
	}

	//! Construct and add string (i.e. make a copy of string)
	void AddMemberString( const CharType* name, const CharType* s, MemSize length )
	{
		CJSONValue<encoding,allocator> valueObject( s, length );
		AddMember( name, valueObject );
	}

	//! Construct and add string (i.e. make a copy of string)
	void AddMemberString( const CharType* name, const CharType* s )
	{		
		CJSONValue<encoding,allocator> valueObject( s );
		AddMember( name, valueObject );
	}

private:
	rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>* GetContextPtr() const { return &CJSONBasic<encoding, allocator>::GetContext(); }

};

template<class encoding, class allocator>
class CJSONObjectRef: public CJSONBasicRef<encoding,allocator>
{

public:
	typedef typename encoding::Ch CharType;

	template<class encoding_writer, class allocator_writer> friend class CJSONSimpleWriter;
	template<class encoding_writer, class allocator_writer> friend class CJSONPrettyWriter;

	CJSONObjectRef( const CJSONObject<encoding, allocator>& base ): CJSONBasicRef<encoding, allocator>( base.GetContextPtr() ){}
	CJSONObjectRef( const CJSONBasicRef<encoding, allocator>& base ): CJSONBasicRef<encoding, allocator>( base )
	{
		if( CJSONBasicRef<encoding, allocator>::GetType() != JSON_Object)
		{
			CJSONBasicRef<encoding, allocator>::m_contextRef = NULL;
		}
	}

	CJSONBasicRef<encoding,allocator> GetMember( const CharType* name ) const
	{
		return CJSONBasicRef<encoding, allocator>::GetMemberBasic( name );
	}

	Bool HasMember( const CharType* name ) const
	{ 
		return CJSONBasicRef<encoding, allocator>::GetContext().HasMember( name );
	}

	MemSize GetMembers( THashMap< TString<CharType>, CJSONBasicRef<encoding,allocator> >& members )
	{
		return CJSONBasicRef<encoding,allocator>::GetMembersBasic( members );
	}

};

typedef CJSONObject< rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json) > CJSONObjectUTF8;
typedef CJSONObjectRef< rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json) > CJSONObjectRefUTF8;

typedef CJSONObject< rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json) > CJSONObjectUTF16;
typedef CJSONObjectRef< rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json) > CJSONObjectRefUTF16;