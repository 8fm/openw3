/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiStorage.h"
#include "behTreeInstance.h"

class CAIStorageExternalPtr
{
protected:
	CAIStorageItem*					m_ptr;									// This object is based on lazy initialization
	THandle< CBehTreeInstance >		m_storage;

public:
	CAIStorageExternalPtr( const CAIStorageItem::CInitializer& initializer, CBehTreeInstance* externalStorage );
	CAIStorageExternalPtr( CName itemName, const IRTTIType* itemType, CBehTreeInstance* externalStorage );

	CAIStorageExternalPtr()
		: m_ptr( NULL )
		, m_storage( NULL )													{}
	CAIStorageExternalPtr( const CAIStorageExternalPtr& p );
	CAIStorageExternalPtr( CAIStorageExternalPtr&& p );
	~CAIStorageExternalPtr()												{ Clear(); }

	void* Get() const														{ if ( m_ptr && m_storage.Get() ) { return m_ptr->Item(); } return NULL; }
	template < class T >
	T* TGet() const															{ if ( m_ptr && m_storage.Get() ) { return m_ptr->GetPtr< T >(); } return NULL; }

	CBehTreeInstance* GetExternalOwner() const								{ return m_storage.Get(); }

	CAIStorageExternalPtr& operator=( const CAIStorageExternalPtr& ptr );
	CAIStorageExternalPtr& operator=( CAIStorageExternalPtr&& ptr );
	void Clear();
};


template < class T >
class TAIStorageExternalPtr : public CAIStorageExternalPtr
{
	typedef CAIStorageExternalPtr Super;
protected:
	T* Item() const															{ void* p = Get(); return p ? static_cast< T* >( p ) : NULL; }
public:
	TAIStorageExternalPtr( const CAIStorageItem::CInitializer& initializer, CBehTreeInstance* externalStorage )
		: Super( initializer, externalStorage )								{}
	TAIStorageExternalPtr(  CName itemName, CBehTreeInstance* externalStorage )
	{
		static IRTTIType* myType = SRTTI::GetInstance().FindType( GetTypeName< T >() );
		if ( myType )
		{
			*this = Move( TAIStorageExternalPtr( itemName, myType, externalStorage ) );
		}
	}
	TAIStorageExternalPtr( CName itemName, const IRTTIType* itemType, CBehTreeInstance* externalStorage )
		: Super( itemName, itemType, externalStorage )						{}

	TAIStorageExternalPtr()
		: Super()															{}
	TAIStorageExternalPtr( const TAIStorageExternalPtr& p )
		: Super( p )														{}
	TAIStorageExternalPtr( TAIStorageExternalPtr&& p )
		: Super( Move( p ) )												{}

	RED_INLINE TAIStorageExternalPtr& operator=( const TAIStorageExternalPtr& ptr )
																			{ Super::operator=( ptr ); return *this; }
	RED_INLINE TAIStorageExternalPtr& operator=( TAIStorageExternalPtr&& ptr )
																			{ Super::operator=( Move( ptr ) ); return *this; }

	RED_INLINE T* operator->() const										{ return Item(); }
	RED_INLINE T& operator*() const										{ return *Item(); }

	RED_INLINE operator T* () const										{ return Item(); }
	RED_INLINE operator bool () const										{ return m_ptr && m_storage.Get(); }
};