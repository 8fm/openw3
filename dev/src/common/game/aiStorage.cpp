#include "build.h"
#include "aiStorage.h"


////////////////////////////////////////////////////////////////////////////
// CAIStorageItem
////////////////////////////////////////////////////////////////////////////
CAIStorageItem* CAIStorageItem::Construct( IRTTIType* type, CName itemName )
{
	Uint32 itemSize = type->GetSize();
	Uint32 baseSize = sizeof( CAIStorageItem );
	const Uint32 totalSize = itemSize+baseSize;
	
	void* mem = RED_MEMORY_ALLOCATE_HYBRID( MemoryPool_Default, MC_AI, totalSize );
	CAIStorageItem* item = new (mem) CAIStorageItem();
	
	item->m_itemType = type;
	item->m_itemName = itemName;
	item->m_flags = AISTORAGE_DEFAULT;
	item->m_refCount = 0;
	type->Construct( item->Item() );
	return item;
}
void CAIStorageItem::Destroy( CAIStorageItem* item )
{
	item->m_itemType->Destruct( item->m_item );
	RED_MEMORY_FREE_HYBRID( MemoryPool_Default, MC_AI, item );
	
}
Bool CAIStorageItem::SupportType( const IRTTIType* t ) const
{
	return ::SRTTI::GetInstance().CanCast( t, m_itemType );
}

/////////////////////////////////////////////////////////////////////////////
// CInitializer && derivatives
/////////////////////////////////////////////////////////////////////////////
void CAIStorageItem::CInitializer::InitializeItem( CAIStorageItem& item ) const
{

}

CName CAIStorageItem::CNamedInitializer::GetItemName() const
{
	return m_storageName;
}


////////////////////////////////////////////////////////////////////////////
// CAIStorage
////////////////////////////////////////////////////////////////////////////
CAIStorage::CAIStorage()
{

}
CAIStorage::~CAIStorage()
{
	
}

void CAIStorage::Clear()
{
	for ( auto it = m_storage.Begin(), end = m_storage.End(); it != end; ++it )
	{
		CAIStorageItem::Destroy( *it );
	}
	m_storage.ClearFast();
}

CAIStorageItem* CAIStorage::RequestItem( const CAIStorageItem::CInitializer& initializer )
{
	IRTTIType* type = initializer.GetItemType();
	CName itemName = initializer.GetItemName();
	auto it = m_storage.Find( itemName );

	if ( it == m_storage.End() )
	{
		it = m_storage.Insert( CAIStorageItem::Construct( type, itemName ) );
		initializer.InitializeItem( **it );
	}
	else
	{
		if ( !(*it)->SupportType( type ) )
		{
			// only chance that method can fail. Field is already initialized but its type is mismatch!
			HALT( "Problem when using ai storage! Item type mismatch.", itemName.AsString().AsChar() );
			return NULL;
		}
	}
	return *it;
}

CAIStorageItem* CAIStorage::GetItem( CName name ) const
{
	auto itFind = m_storage.Find( name );
	if ( itFind != m_storage.End() )
	{
		return *itFind;
	}
	return NULL;
}


void CAIStorage::DeleteItem( CName name )
{
	auto it = m_storage.Find( name );
	ASSERT( it != m_storage.End() );
	CAIStorageItem::Destroy( *it );
	m_storage.Erase( it );
}

void CAIStorage::OnGarbageCollector( IFile& file )
{
	for ( auto it = m_storage.Begin(), end = m_storage.End(); it != end; ++it )
	{
		CAIStorageItem* item = *it;
		if ( item->GetFlags() & AISTORAGE_GARBAGECOLLECTED )
		{
			if ( item->GetFlags() & AISTORAGE_VIRTUALINTERFACE )
			{
				static_cast< CAIStorageItemVirtualInterface* >( item->Item() )->CustomSerialize( file );
			}
			else
			{
				item->GetType()->Serialize( file, item->Item() );
			}
		}
	}
}
void CAIStorage::OnDetached()
{
	for ( auto it = m_storage.Begin(), end = m_storage.End(); it != end; ++it )
	{
		CAIStorageItem* item = *it;
		if ( item->GetFlags() & AISTORAGE_VIRTUALINTERFACE )
		{
			static_cast< CAIStorageItemVirtualInterface* >( item->Item() )->OnDetached();
		}
	}
}


////////////////////////////////////////////////////////////////////////////
// CAIStoragePtr
////////////////////////////////////////////////////////////////////////////
CAIStoragePtr::CAIStoragePtr( CName itemName, const IRTTIType* itemType, CAIStorage* storage )
	: m_storage( storage )
{
	CAIStorageItem* item = m_storage->GetItem( itemName );
	if ( item && item->SupportType( itemType ) )
	{
		item->AddRef();
		m_ptr = item;
	}
}
CAIStoragePtr::CAIStoragePtr( const CAIStorageItem::CInitializer& initializer, CAIStorage* storage )
	: m_storage( storage )
{
	m_ptr = storage->RequestItem( initializer );
	if ( m_ptr )
	{
		m_ptr->AddRef();
	}
}

////////////////////////////////////////////////////////////////////////////
// CAIStorageItemVirtualInterface
////////////////////////////////////////////////////////////////////////////
void CAIStorageItemVirtualInterface::CInitializer::InitializeItem( CAIStorageItem& item ) const
{
	AddFlags( item, AISTORAGE_VIRTUALINTERFACE );
}
void CAIStorageItemVirtualInterface::CustomSerialize( IFile& file )
{

}
void CAIStorageItemVirtualInterface::OnDetached()
{

}