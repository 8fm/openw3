/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "jsonBasic.h"

template<class encoding, class allocator>
class CJSONValue: public CJSONBasic<encoding, allocator>
{
public:
	typedef typename encoding::Ch CharType;

	template<class encoding_value, class allocator_value> friend class CJSONValueRef;

	CJSONValue(): CJSONBasic<encoding, allocator>(new rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>( (rapidjson::Type) JSON_Null ) ) {}

	//! Constructor for bool value.
	CJSONValue(Bool b): CJSONBasic<encoding, allocator>( new rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>( b ) ) {}

	//! Constructor for int value.
	CJSONValue(Int32 i): CJSONBasic<encoding, allocator>( new rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>( i ) ) {}

	//! Constructor for unsigned value.
	CJSONValue(Uint32 u): CJSONBasic<encoding, allocator>(new  rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>( u ) ) {}

	//! Constructor for int64_t value.
	CJSONValue(Int64 i64): CJSONBasic<encoding, allocator>(new rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>( i64 ) ) {}

	//! Constructor for uint64_t value.
	CJSONValue(Uint64 u64): CJSONBasic<encoding, allocator>(new rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>( u64 ) ) {}

	//! Constructor for double value.
	CJSONValue(Double d): CJSONBasic<encoding, allocator>(new rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>( d ) ) {}

	//! Constructor for constant string (i.e. make a copy of string)
	CJSONValue(const CharType* s, MemSize length): CJSONBasic<encoding, allocator>(new rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>( s, (rapidjson::SizeType)length, allocator::PoolAllocatorInstance::GetInstance() ) ) {}

	//! Constructor for constant string (i.e. make a copy of string)
	CJSONValue(const CharType* s): CJSONBasic<encoding, allocator>(new rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>( s, allocator::PoolAllocatorInstance::GetInstance() ) ) {}

private:
	rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>* GetContextPtr() const { return &CJSONBasic<encoding, allocator>::GetContext(); }

};

template<class encoding, class allocator>
class CJSONValueRef: public CJSONBasicRef<encoding, allocator>
{
public:
	typedef typename encoding::Ch CharType;

public:
	CJSONValueRef( const CJSONValue<encoding, allocator>& base ): CJSONBasicRef<encoding, allocator>( base.GetContextPtr() ){}
	CJSONValueRef( const CJSONBasicRef<encoding, allocator>& base ): CJSONBasicRef<encoding, allocator>( base )
	{
		if(    CJSONBasicRef<encoding, allocator>::GetType() == JSON_Object
			|| CJSONBasicRef<encoding, allocator>::GetType() == JSON_Array)
		{
			CJSONBasicRef<encoding, allocator>::m_contextRef = NULL;
		}
	}
		
	Bool IsBool()   const { return CJSONBasicRef<encoding, allocator>::GetContext().IsBool(); }
	Bool IsInt32()  const { return CJSONBasicRef<encoding, allocator>::GetContext().IsInt(); }
	Bool IsUint32() const { return CJSONBasicRef<encoding, allocator>::GetContext().IsUint(); }
	Bool IsInt64()  const { return CJSONBasicRef<encoding, allocator>::GetContext().IsInt64(); }
	Bool IsUint64() const { return CJSONBasicRef<encoding, allocator>::GetContext().IsUint64(); }
	Bool IsDouble() const { return CJSONBasicRef<encoding, allocator>::GetContext().IsDouble(); }
	Bool IsString() const { return CJSONBasicRef<encoding, allocator>::GetContext().IsString(); }

	Bool	GetBool()   const { return CJSONBasicRef<encoding, allocator>::GetContext().GetBool(); }
	Int32	GetInt32()  const { return CJSONBasicRef<encoding, allocator>::GetContext().GetInt(); }
	Uint32	GetUint32() const { return CJSONBasicRef<encoding, allocator>::GetContext().GetUint(); }
	Int64	GetInt64()  const { return CJSONBasicRef<encoding, allocator>::GetContext().GetInt64(); }
	Uint64	GetUint64() const { return CJSONBasicRef<encoding, allocator>::GetContext().GetUint64(); }
	Double	GetDouble() const { return CJSONBasicRef<encoding, allocator>::GetContext().GetDouble(); }
	const CharType* GetString() const { return CJSONBasicRef<encoding, allocator>::GetContext().GetString(); }
};

typedef CJSONValue< rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json) > CJSONValueUTF8;
typedef CJSONValueRef< rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json) > CJSONValueRefUTF8;

typedef CJSONValue< rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json) > CJSONValueUTF16;
typedef CJSONValueRef< rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json) > CJSONValueRefUTF16;