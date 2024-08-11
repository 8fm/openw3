/*
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "aiScriptableStorage.h"

///////////////////////////////////////////////////////////////////////////////
// CAIScriptableStorage
///////////////////////////////////////////////////////////////////////////////
void CAIScriptableStorage::OnGarbageCollector( IFile& file )
{
	RED_ASSERT( file.IsGarbageCollector() );
	for ( const auto& pair : m_storage )
	{
		pair.m_second->OnSerialize( file );
	}
}

IScriptable* CAIScriptableStorage::ScriptableStorageFindItem( CName itemName )
{
	auto it = m_storage.Find( itemName );
	if ( it == m_storage.End() )
	{
		return nullptr;
	}
	IScriptable* ret = it->m_second.Get();
	if ( !ret )
	{
		return nullptr;
	}
	return ret;
}

IScriptable* CAIScriptableStorage::ScriptableStorageRequestItem( CName itemName, const CClass* itemClass )
{
	auto it = m_storage.Find( itemName );
	if ( it != m_storage.End() )
	{
		IScriptable* ret = it->m_second.Get();
		if ( ret )
		{
			if ( !ret->GetClass()->IsA( itemClass ) )
			{
				return nullptr;
			}

			// the most likely execution case
			return ret;
		}
		else
		{
			// shouldn't ever hit
			m_storage.Erase( it );
		}
	}

	if ( !itemClass->IsScriptable() )
	{
		return nullptr;
	}

	THandle< IScriptable > objHandle = itemClass->CreateObject< IScriptable >();
	IScriptable* ret = objHandle.Get();
	ret->EnableReferenceCounting( true );
	// create new element
	m_storage.Insert( itemName, Move( objHandle ) );
	return ret;
}
IScriptable* CAIScriptableStorage::ScriptableStorageFindItem( CName itemName, const CClass* itemClass )
{
	IScriptable* ret = ScriptableStorageFindItem( itemName );
	if ( !ret )
	{
		return nullptr;
	}
	if ( !ret->GetClass()->IsA( itemClass ) )
	{
		return nullptr;
	}
	return ret;
}


IScriptable* CAIScriptableStorage::ScriptableStorageRequestItem( CName itemName, CName itemType )
{
	CClass* typeDef = SRTTI::GetInstance().FindClass( itemType );
	if ( !typeDef )
	{
		return nullptr;
	}
	return ScriptableStorageRequestItem( itemName, typeDef );
}

IScriptable* CAIScriptableStorage::ScriptableStorageFindItem( CName itemName, CName itemType )
{
	CClass* typeDef = SRTTI::GetInstance().FindClass( itemType );
	if ( !typeDef )
	{
		return nullptr;
	}
	return ScriptableStorageFindItem( itemName, typeDef );
}

void CAIScriptableStorage::ScriptableStorageReleaseItem( CName itemName )
{
	m_storage.Erase( itemName );
}

///////////////////////////////////////////////////////////////////////////////
// CAIScriptableStorageItemPtr
///////////////////////////////////////////////////////////////////////////////
CAIScriptableStorageItemPtr::CAIScriptableStorageItemPtr( CName itemName, const CClass* itemType, CAIScriptableStorage* storage )
{
	m_scriptable = storage->ScriptableStorageRequestItem( itemName, itemType );
}
