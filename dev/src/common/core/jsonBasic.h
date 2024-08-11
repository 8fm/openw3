/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "rapidjson/document.h"
#include "jsonAllocator.h"
#include "hashmap.h"

enum EJSONType
{
	JSON_Null   = 0,	//!< null
	JSON_False  = 1,	//!< false
	JSON_True   = 2,	//!< true
	JSON_Object = 3,	//!< object
	JSON_Array  = 4,	//!< array 
	JSON_String = 5,	//!< string
	JSON_Number = 6,	//!< number
};

template<class encoding, class allocator>
class CJSONBasic
{
private:
	CJSONBasic( const CJSONBasic<encoding, allocator>& other );

public:
	typedef typename encoding::Ch CharType;

	virtual ~CJSONBasic()
	{ 
		delete m_context;
	}

protected:

	CJSONBasic( rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>* context ): m_context( context ){}

	rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>& GetContext() const 
	{ 
		return *m_context;
	}

	void AddMemberBasic( const CharType* name, const CJSONBasic<encoding, allocator>& object )
	{		
		GetContext().AddMember( name, object.GetContext(), allocator::PoolAllocatorInstance::GetInstance() );
	}

	void PushBackBasic( const CJSONBasic<encoding, allocator>& object )
	{
		GetContext().PushBack( object.GetContext(), allocator::PoolAllocatorInstance::GetInstance() );
	}

public: 
	rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>* m_context;

};

template<class encoding, class allocator>
class CJSONBasicRef
{
public:
	CJSONBasicRef( const CJSONBasicRef& other ): m_contextRef( other.m_contextRef ){}

public:
	typedef typename encoding::Ch CharType;

	template<class encoding_documnt, class allocator_documnt> friend class CJSONDocument;

	EJSONType GetType() const
	{ 
		return (EJSONType)GetContext().GetType();
	}

protected:
	CJSONBasicRef( rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>* context ): m_contextRef( context ){}

	rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>& GetContext() const
	{ 
		return *m_contextRef;
	}

	CJSONBasicRef<encoding, allocator> GetMemberBasic( const CharType* name ) const
	{
		if( GetContext().HasMember( name ) == true )
		{
			return CJSONBasicRef<encoding, allocator>( &GetContext()[name] );
		}
		return CJSONBasicRef<encoding, allocator>(NULL);
	}

	MemSize GetMembersBasic( THashMap< TString<CharType>, CJSONBasicRef<encoding,allocator> >& members ) const
	{
		typename rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>::MemberIterator end = GetContext().MemberEnd();
		typename rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>::MemberIterator it  = GetContext().MemberBegin();
		MemSize count =0;
		for( ; it != end; ++it )
		{
			members.Set( (*it).name.GetString(), CJSONBasicRef<encoding,allocator>( &((*it).value) ) );
			count++;
		}
		return count;
	}

	CJSONBasicRef<encoding, allocator> GetMemberAtBasic( const MemSize index ) const
	{
		return CJSONBasicRef<encoding, allocator>( &GetContext()[(rapidjson::SizeType)index] );
	}
		
protected: 
	rapidjson::GenericValue<encoding, typename allocator::PoolAllocator>* m_contextRef;

};
