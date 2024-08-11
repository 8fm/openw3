#pragma once

RED_WARNING_PUSH()
RED_DISABLE_WARNING_MSC( 4200 )												// nonstandard extension used : zero-sized array in struct/union

class CAIStorage;

enum EAIStorageFlags
{
	AISTORAGE_DEFAULT				= 0,
	AISTORAGE_SAVEABLE				= FLAG(0),
	AISTORAGE_GARBAGECOLLECTED		= FLAG(1),
	AISTORAGE_VIRTUALINTERFACE		= FLAG(2)
};

////////////////////////////////////////////////////////////////////////////
// Custom AI storage item that can be.. just everything!
class CAIStorageItem
{
protected:
	CAIStorageItem()														{}
	~CAIStorageItem()														{}

	IRTTIType*			m_itemType;
	CName				m_itemName;
	Uint16				m_flags;
	Int16				m_refCount;
	Uint32				m_item[];

public:
	class CInitializer
	{
	public:
		virtual CName GetItemName() const									= 0;
		virtual void InitializeItem( CAIStorageItem& item ) const;
		virtual IRTTIType* GetItemType() const								= 0;

	protected:
		void AddFlags( CAIStorageItem& item, Uint16 flags )	const			{ item.m_flags |= flags; }
	};

	class CNamedInitializer : public CInitializer
	{
	protected:
		CName			m_storageName;
	public:
		CNamedInitializer( CName name )
			: m_storageName( name )											{}
		CName GetItemName() const override;
	};

	static CAIStorageItem* Construct( IRTTIType* type, CName itemName );
	static void Destroy( CAIStorageItem* item );

	RED_INLINE void AddRef()												{ ++m_refCount; }
	RED_INLINE void DelRef( CAIStorage* storage );

	RED_INLINE CName GetName() const										{ return m_itemName; }
	RED_INLINE void* Item()													{ return reinterpret_cast< void* >( &m_item[0] ); }

	Bool SupportType( const IRTTIType* t ) const;

	template < class T >
	RED_INLINE T* GetPtr();

	RED_INLINE IRTTIType* GetType() const									{ return m_itemType; }
	RED_INLINE Uint16 GetFlags() const										{ return m_flags; }
};

////////////////////////////////////////////////////////////////////////////
// Storage of absolutely
class CAIStorage
{
protected:
	struct StorageOrder
	{
		static RED_INLINE Bool Less( CAIStorageItem* i1, CAIStorageItem* i2 )
		{
			return i1->GetName() < i2->GetName();
		}
		static RED_INLINE Bool Less( CName i1, CAIStorageItem* i2 )
		{
			return i1 < i2->GetName();
		}
		static RED_INLINE Bool Less( CAIStorageItem* i1, CName i2 )
		{
			return i1->GetName() < i2;
		}
	};
	TSortedArray< CAIStorageItem*, StorageOrder >		m_storage;

	void Clear();

public:
	CAIStorage();
	~CAIStorage();

	CAIStorageItem* RequestItem( const CAIStorageItem::CInitializer& initializer );

	CAIStorageItem* GetItem( CName name ) const;

	template < class T >
	T* GetTypedItem( CName itemName );

	void DeleteItem( CName name );

	void OnGarbageCollector( IFile& file );
	void OnDetached();
};


////////////////////////////////////////////////////////////////////////////
// Storage item safe handler
class CAIStoragePtr
{
protected:
	CAIStorageItem*		m_ptr;												// This object is based on lazy initialization
	CAIStorage*			m_storage;

public:
	CAIStoragePtr( const CAIStorageItem::CInitializer& initializer, CAIStorage* storage );
	CAIStoragePtr( CName itemName, const IRTTIType* itemType, CAIStorage* storage );

	CAIStoragePtr()
		: m_ptr( NULL )
		, m_storage( NULL )													{}
	CAIStoragePtr( const CAIStoragePtr& p )
		: m_ptr( p.m_ptr )
		, m_storage( p.m_storage )											{ if ( m_ptr ) { m_ptr->AddRef(); } }

	CAIStoragePtr( CAIStoragePtr&& p )
		: m_ptr( p.m_ptr )
		, m_storage( p.m_storage )											{ p.m_ptr = NULL; }

	~CAIStoragePtr()														{ if ( m_ptr ) { m_ptr->DelRef( m_storage ); } }

	void* Get()																{ if ( m_ptr ) { return m_ptr->Item(); } return NULL; }
	template < class T >
	T* TGet()																{ if ( m_ptr ) { return m_ptr->GetPtr< T >(); } return NULL; }

	RED_INLINE CAIStoragePtr& operator=( const CAIStoragePtr& ptr )			{ if ( this != &ptr ) { m_ptr = ptr.m_ptr; m_storage = ptr.m_storage; if ( m_ptr ) { m_ptr->AddRef(); } } return *this; }
	RED_INLINE CAIStoragePtr& operator=( CAIStoragePtr&& ptr )				{ m_ptr = ptr.m_ptr; m_storage = ptr.m_storage; ptr.m_ptr = NULL; return *this; }
	RED_INLINE void Clear()													{ if ( m_ptr ) { m_ptr->DelRef( m_storage ); m_ptr = NULL; m_storage = NULL; } }

	void WasInvalidatedExternally()											{ m_ptr = NULL; m_storage = NULL; }
};


////////////////////////////////////////////////////////////////////////////
// Typed storage pointer template
template < class T >
class TAIStoragePtr : public CAIStoragePtr
{
	typedef CAIStoragePtr Super;
protected:
	T* Item() const															{ return m_ptr ? static_cast< T* >( m_ptr->Item() ) : NULL; }
public:
	TAIStoragePtr( const CAIStorageItem::CInitializer& initializer, CAIStorage* storage )
		: Super( initializer, storage )										{}

	TAIStoragePtr()
		: Super()															{}
	TAIStoragePtr( const TAIStoragePtr& p )
		: Super( p )														{}
	TAIStoragePtr( TAIStoragePtr&& p )
		: Super( Move( p ) )												{}

	RED_INLINE TAIStoragePtr& operator=( const TAIStoragePtr& ptr )		{ Super::operator=( ptr ); return *this; }
	RED_INLINE TAIStoragePtr& operator=( TAIStoragePtr&& ptr )			{ Super::operator=( Move( ptr ) ); return *this; }

	RED_INLINE T* operator->() const										{ return Item(); }
	RED_INLINE T& operator*() const										{ return *Item(); }

	RED_INLINE void* RawGet()												{ return Super::Get(); }
	RED_INLINE T* Get() const												{ return Item(); }
	RED_INLINE operator bool () const										{ return Item() != NULL; }
};

////////////////////////////////////////////////////////////////////////////
// Class supporting custom serializer
class CAIStorageItemVirtualInterface
{
public:
	class CInitializer : public CAIStorageItem::CInitializer
	{
	public:
		void InitializeItem( CAIStorageItem& item ) const override;
	};

	virtual void CustomSerialize( IFile& file );
	virtual void OnDetached();
};

////////////////////////////////////////////////////////////////////////////
// Inlined implementations

template < class T >
T* CAIStorageItem::GetPtr()
{
	static IRTTIType const* t = SRTTI::GetInstance().FindType( GetTypeName< T >() );
	if ( SupportType( t ) )
	{
		return reinterpret_cast< T* >( m_item );
	}
	return NULL;
}

void CAIStorageItem::DelRef( CAIStorage* storage )
{
	if ( --m_refCount <= 0 )
	{
		storage->DeleteItem( m_itemName );
	}
}

template < class T >
T* CAIStorage::GetTypedItem( CName itemName )
{
	CAIStorageItem* storageItem = GetItem( itemName );
	return storageItem ? storageItem->GetPtr< T >() : NULL;
}

RED_WARNING_POP()
