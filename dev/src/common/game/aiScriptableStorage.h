/*
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#pragma once



class CAIScriptableStorage
{
public:
	CAIScriptableStorage()																				{}
	~CAIScriptableStorage()																				{}

	void			OnGarbageCollector( IFile& file );

	IScriptable*	ScriptableStorageFindItem( CName itemName );

	IScriptable*	ScriptableStorageRequestItem( CName itemName, CName itemType );
	IScriptable*	ScriptableStorageFindItem( CName itemName, CName itemType );

	IScriptable*	ScriptableStorageRequestItem( CName itemName, const CClass* itemClass );
	IScriptable*	ScriptableStorageFindItem( CName itemName, const CClass* itemClass );
	void			ScriptableStorageReleaseItem( CName itemName );

	template < class T >
	T*				TScriptableStoragetFindItem( CName itemName	)										{ return static_cast< T* >( ScriptableStorageFindItem( itemName, T::GetStaticClass() ) ); }

protected:
	TArrayMap< CName, THandle< IScriptable > >			m_storage;
};

// syntactic sugar, that will make transformation from AIStorage things to scriptable storage easy
class CAIScriptableStorageItemPtr
{
public:
	CAIScriptableStorageItemPtr( CName itemName, const CClass* itemType, CAIScriptableStorage* storage );

	CAIScriptableStorageItemPtr()											{}
	~CAIScriptableStorageItemPtr()											{}

	IScriptable* Get() const												{ return m_scriptable.Get(); }
	template < class T >
	T* TGet() const															{ return ::Cast< T >( m_scriptable.Get() ); }

	RED_INLINE void Clear()													{ m_scriptable = nullptr; }

protected:
	THandle< IScriptable >								m_scriptable;

};

template < class T >
class TAIScriptableStorageItemPtr : public CAIScriptableStorageItemPtr
{
	typedef CAIScriptableStorageItemPtr Super;
protected:
	T* Item() const															{ return static_cast< T* >( m_scriptable.Get() ); }
public:
	TAIScriptableStorageItemPtr( CAIScriptableStorage* storage, CName itemName )
		: Super( itemName, T::GetStaticClass(), storage )					{}
	TAIScriptableStorageItemPtr()											{}

	RED_INLINE T* operator->() const										{ return Item(); }
	RED_INLINE T& operator*() const											{ return *Item(); }

	RED_INLINE void* RawGet()												{ return Super::Get(); }
	RED_INLINE T* Get() const												{ return Item(); }
	RED_INLINE operator bool () const										{ return Item() != NULL; }
};