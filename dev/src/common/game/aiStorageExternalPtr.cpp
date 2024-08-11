/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "aiStorageExternalPtr.h"

#include "behTreeInstance.h"

////////////////////////////////////////////////////////////////////////////
// CAIStorageExternalPtr
////////////////////////////////////////////////////////////////////////////
CAIStorageExternalPtr::CAIStorageExternalPtr( const CAIStorageItem::CInitializer& initializer, CBehTreeInstance* externalStorage )
{
	m_ptr = externalStorage->RequestItem( initializer );
	if ( m_ptr )
	{
		m_ptr->AddRef();
		m_storage = externalStorage;
	}
}
CAIStorageExternalPtr::CAIStorageExternalPtr( CName itemName, const IRTTIType* itemType, CBehTreeInstance* externalStorage )
	: m_ptr( NULL )
	, m_storage( externalStorage )
{
	CAIStorageItem* item = externalStorage->GetItem( itemName );

	if ( item && item->SupportType( itemType ) )
	{
		item->AddRef();
		m_ptr = item;
		m_storage = externalStorage;
	}
}
CAIStorageExternalPtr::CAIStorageExternalPtr( const CAIStorageExternalPtr& p )
	: m_ptr( p.m_ptr )
	, m_storage( p.m_storage )
{
	if ( m_storage.Get() && m_ptr )
	{
		m_ptr->AddRef();
	}
}
CAIStorageExternalPtr::CAIStorageExternalPtr( CAIStorageExternalPtr&& p )
	: m_ptr( p.m_ptr )
	, m_storage( p.m_storage )
{
	p.m_ptr = NULL;
	p.m_storage = NULL;
}

CAIStorageExternalPtr& CAIStorageExternalPtr::operator=( const CAIStorageExternalPtr& p )
{
	if ( p.m_ptr != m_ptr )
	{
		{
			CAIStorage* storage = m_storage.Get();
			if ( storage && m_ptr )
			{
				m_ptr->DelRef( storage );
			}
		}

		m_ptr = p.m_ptr;
		m_storage = p.m_storage;
		if ( m_storage.Get() && m_ptr )
		{
			m_ptr->AddRef();
		}
	}
	return *this;
}
CAIStorageExternalPtr& CAIStorageExternalPtr::operator=( CAIStorageExternalPtr&& p )
{
	{
		CAIStorage* storage = m_storage.Get();
		if ( storage && m_ptr )
		{
			m_ptr->DelRef( storage );
		}
	}
	m_ptr = p.m_ptr;
	m_storage = p.m_storage;
	p.m_ptr = NULL;
	p.m_storage = NULL;
	return *this;
}
void CAIStorageExternalPtr::Clear()
{
	{
		CAIStorage* storage = m_storage.Get();
		if ( storage && m_ptr )
		{
			m_ptr->DelRef( storage );
		}
	}
	m_ptr = NULL;
	m_storage = NULL;
}