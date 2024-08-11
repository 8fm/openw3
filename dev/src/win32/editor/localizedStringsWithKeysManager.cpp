// Copyright © 2015 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "localizedStringsWithKeysManager.h"
#include "sqlSelectStringsWithKeys.h"
#include "odbcMgr.h"
#include "../../common/engine/localizationManager.h"

// =================================================================================================
namespace {
// =================================================================================================

/*
Retrieves from String DB all strings whose STRING_KEY is not null.
*/
void QueryStringsWithKeys( TDynArray< SqlSelectStringsWithKeys::ResultsetRow >& outResult )
{
	COdbcMgr odbcMgr;
	if( odbcMgr.Initialize() )
	{
		SQLHDBC connRedDb = ConnectRedDb( odbcMgr );

		SqlSelectStringsWithKeys querySelectStringsWithKeys;
		querySelectStringsWithKeys.Prepare( connRedDb );
		querySelectStringsWithKeys.Execute( outResult );

		querySelectStringsWithKeys.Reset();
		odbcMgr.Disconnect( connRedDb );
		odbcMgr.Uninitialize();
	}
}

// =================================================================================================
} // unnamed namespace
// =================================================================================================

void CLocalizedStringsWithKeys::GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings )
{
	typedef TDynArray< SLocStringWithKeyEntry >::iterator LocStringsI;
	for ( LocStringsI locStringI = m_locStringsEntries.Begin();
		locStringI != m_locStringsEntries.End(); 
		++locStringI )
	{
		if ( locStringI->m_isModified )
		{
			if ( locStringI->m_categoryIndex >= (Int32)m_locStringsCategories.Size() )
			{
				// WARNING
				continue;
			}



			String category;
			if  ( locStringI->m_categoryIndex < 0 )
			{
				category = String::EMPTY;
			}
			else
			{
				category = m_locStringsCategories[ locStringI->m_categoryIndex ];
			}

			// don't save entries with empty keys
			if ( !locStringI->m_isMarkedForRemove && locStringI->m_stringKey == String::EMPTY )
			{
				continue;
			}

			if ( locStringI->m_isMarkedForRemove )
			{
				locStringI->m_stringKey = String::EMPTY;
			}

			LocalizedStringEntry entry( &locStringI->m_locString, category, NULL, String::EMPTY, locStringI->m_stringKey );
			localizedStrings.PushBack( entry );

			locStringI->m_isModified = false; // treat this as saved

			// TODO: remove from data removed strings
		}
	}
}

void CLocalizedStringsWithKeys::Initialize()
{
	// Discard all changes
	m_locStringsCategories.Clear();
	m_locStringsEntries.Clear();

	TDynArray< SqlSelectStringsWithKeys::ResultsetRow > stringsWithKeys;
	QueryStringsWithKeys( stringsWithKeys );

	for( const SqlSelectStringsWithKeys::ResultsetRow& row : stringsWithKeys )
	{
		AddLocStringEntry( row.m_stringId, row.m_stringKey, row.m_stringPropertyName );
	}
}

Bool CLocalizedStringsWithKeys::AddLocStringEntry( Uint32 stringId, const String &stringKey, const String &stringCategory )
{
	if ( stringId == 0 ) return false;
	if ( stringKey == String::EMPTY ) return false;
	if ( stringCategory == String::EMPTY ) return false;
	if ( DoesLocStringEntryExist( stringId, stringKey ) ) return false; // comment this for optimization

	SLocStringWithKeyEntry entry;

	Int32 categoryIndex = UpdateCategory( stringCategory );
	if ( categoryIndex < 0 ) return false;

	entry.m_locString.SetIndex( stringId );
	entry.m_stringKey = stringKey;
	entry.m_categoryIndex = categoryIndex;
	entry.m_isModified = false;
	entry.m_isMarkedForRemove = false;

	m_locStringsEntries.PushBack( entry );

	return true;
}

Bool CLocalizedStringsWithKeys::UpdateLocStringEntry( Uint32 stringId, const String &stringText, const String &stringKey, const String &stringCategory )
{
	Int32 locStringsEntryIndex = FindLocStringEntryById( stringId );

	// Update existing entry
	if ( locStringsEntryIndex != -1 )
	{
		m_locStringsEntries[ locStringsEntryIndex ].m_stringKey = stringKey;
		m_locStringsEntries[ locStringsEntryIndex ].m_isModified = true;
		m_locStringsEntries[ locStringsEntryIndex ].m_locString.SetString( stringText );

		Int32 categoryIndex = UpdateCategory( stringCategory );
		//if ( categoryIndex < 0 )
		//{
		//	return false; // cannot get category (shouldn't happen)
		//}

		m_locStringsEntries[ locStringsEntryIndex ].m_categoryIndex = categoryIndex;

		return true;
	}
	else
	{
		return false; // entry not found
	}
}

Bool CLocalizedStringsWithKeys::UpdateLocStringKey( Uint32 stringIndex, const String &stringKey )
{
	// Update existing entry
	if ( stringIndex < m_locStringsEntries.Size() )
	{
		m_locStringsEntries[ stringIndex ].m_stringKey = stringKey;
		m_locStringsEntries[ stringIndex ].m_isModified = true;

		return true;
	}
	else
	{
		return false; // entry not found
	}
}

Bool CLocalizedStringsWithKeys::RemoveLocString( Uint32 stringIndex )
{
	if ( stringIndex < m_locStringsEntries.Size() )
	{
		m_locStringsEntries[ stringIndex ].m_isMarkedForRemove = true;
		m_locStringsEntries[ stringIndex ].m_isModified = true;

		return true;
	}
	else
	{
		return false; // entry not found
	}
}

Bool CLocalizedStringsWithKeys::UpdateLocStringText( Uint32 stringIndex, const String &stringText )
{
	// Update existing entry
	if ( stringIndex < m_locStringsEntries.Size() )
	{
		m_locStringsEntries[ stringIndex ].m_isModified = true;
		m_locStringsEntries[ stringIndex ].m_locString.SetString( stringText );

		return true;
	}
	else
	{
		return false; // entry not found
	}
}

Bool CLocalizedStringsWithKeys::UpdateLocStringCategory( Uint32 stringIndex, const String &stringCategory )
{
	// Update existing entry
	if ( stringIndex < m_locStringsEntries.Size() )
	{
		m_locStringsEntries[ stringIndex ].m_isModified = true;

		Int32 categoryIndex = UpdateCategory( stringCategory );
		//if ( categoryIndex < 0 )
		//{
		//	return false; // cannot get category (shouldn't happen)
		//}

		m_locStringsEntries[ stringIndex ].m_categoryIndex = categoryIndex;

		return true;
	}
	else
	{
		return false; // entry not found
	}
}

Int32 CLocalizedStringsWithKeys::CreateLocStringEntry( const String &stringText, const String &stringKey, const String &stringCategory )
{
	//if ( stringKey == String::EMPTY ) return false;
	//if ( stringCategory == String::EMPTY ) return false;

	// We cannot create new entry if there is already one with the same string key
	if ( stringKey != String::EMPTY && DoesLocStringEntryExist( stringKey ) ) return 0;

	LocalizedString newLocString;
	newLocString.SetIndex( 0 );
	newLocString.SetString( stringText );

	SLocStringWithKeyEntry entry;

	Int32 categoryIndex = UpdateCategory( stringCategory );
	if ( categoryIndex < 0 ) return 0;

	entry.m_locString = newLocString;
	entry.m_stringKey = stringKey;
	entry.m_categoryIndex = categoryIndex;
	entry.m_isModified = true;
	entry.m_isMarkedForRemove = false;

	m_locStringsEntries.PushBack( entry );

	return (Int32)m_locStringsEntries.Size() - 1;
}

Int32 CLocalizedStringsWithKeys::GetCategoryIndex( const String& category )
{
	return static_cast< Int32 >( m_locStringsCategories.GetIndex( category ) );
}

Int32 CLocalizedStringsWithKeys::UpdateCategory( const String& category )
{
	Int32 index = static_cast< Int32 >( m_locStringsCategories.GetIndex( category ) );

	if ( index == -1 )
	{
		m_locStringsCategories.PushBack( category );
		index = m_locStringsCategories.Size() - 1;
	}

	return index;
}

Bool CLocalizedStringsWithKeys::DoesLocStringEntryExist( Uint32 stringId, const String &stringKey ) const
{
	typedef TDynArray< SLocStringWithKeyEntry >::const_iterator LocStringsCI;
	for ( LocStringsCI locStringCI = m_locStringsEntries.Begin();
		locStringCI != m_locStringsEntries.End(); 
		++locStringCI )
	{
		Bool doesIdMatch = locStringCI->m_locString.GetIndex() == stringId;
		Bool doesStrinKeyMatch = locStringCI->m_stringKey == stringKey;
		if ( doesIdMatch || doesStrinKeyMatch )
		{
			return true;
		}
	}

	return false;
}

Bool CLocalizedStringsWithKeys::DoesLocStringEntryExist( Uint32 stringId ) const
{
	typedef TDynArray< SLocStringWithKeyEntry >::const_iterator LocStringsCI;
	for ( LocStringsCI locStringCI = m_locStringsEntries.Begin();
		locStringCI != m_locStringsEntries.End(); 
		++locStringCI )
	{
		Bool doesIdMatch = locStringCI->m_locString.GetIndex() == stringId;
		if ( doesIdMatch )
		{
			return true;
		}
	}

	return false;
}

Bool CLocalizedStringsWithKeys::DoesLocStringEntryExist( const String &stringKey ) const
{
	typedef TDynArray< SLocStringWithKeyEntry >::const_iterator LocStringsCI;
	for ( LocStringsCI locStringCI = m_locStringsEntries.Begin();
		locStringCI != m_locStringsEntries.End(); 
		++locStringCI )
	{
		Bool doesStrinKeyMatch = locStringCI->m_stringKey == stringKey;
		if ( doesStrinKeyMatch )
		{
			return true;
		}
	}

	return false;
}

Int32 CLocalizedStringsWithKeys::FindLocStringEntryById( Uint32 stringId )
{
	typedef TDynArray< SLocStringWithKeyEntry >::const_iterator LocStringsCI;
	Int32 index = 0;
	for ( LocStringsCI locStringCI = m_locStringsEntries.Begin();
		locStringCI != m_locStringsEntries.End(); 
		++locStringCI, ++index )
	{
		Bool doesIdMatch = locStringCI->m_locString.GetIndex() == stringId;
		if ( doesIdMatch )
		{
			return index;
		}
	}

	return -1;
}

Uint32 CLocalizedStringsWithKeys::GetSize() const
{
	return m_locStringsEntries.Size();
}

LocalizedString CLocalizedStringsWithKeys::GetLocString( Uint32 index ) const
{
	ASSERT( index < GetSize() );

	return m_locStringsEntries[index].m_locString;
}

String CLocalizedStringsWithKeys::GetStringKey( Uint32 index ) const
{
	ASSERT( index < GetSize() );

	return m_locStringsEntries[index].m_stringKey;
}

String CLocalizedStringsWithKeys::GetStringCategory( Uint32 index ) const
{
	ASSERT( index < GetSize() );
	ASSERT( m_locStringsEntries[index].m_categoryIndex < (Int32)m_locStringsCategories.Size() );

	return m_locStringsCategories[ m_locStringsEntries[index].m_categoryIndex ];
}

Uint32 CLocalizedStringsWithKeys::GetStringIndex( Uint32 index ) const
{
	ASSERT( index < GetSize() );

	return m_locStringsEntries[index].m_locString.GetIndex();
}

Bool CLocalizedStringsWithKeys::DoesMatchStringCategory( Uint32 index, const String &category ) const
{
	ASSERT( index < GetSize() );
	ASSERT( m_locStringsEntries[index].m_categoryIndex < (Int32)m_locStringsCategories.Size() );

	// Empty string category matches everything
	if ( category == String::EMPTY )
	{
		return true;
	}

	return m_locStringsCategories[ m_locStringsEntries[index].m_categoryIndex ] == category;
}

Bool CLocalizedStringsWithKeys::IsMarkedForDelete( Uint32 index ) const
{
	ASSERT( index < GetSize() );

	return m_locStringsEntries[index].m_isMarkedForRemove;
}

Bool CLocalizedStringsWithKeys::DoesKeyExist( Int32 baseIndex, const String &key )
{
	typedef TDynArray< SLocStringWithKeyEntry >::const_iterator LocStringsCI;
	Int32 index = 0;
	for ( LocStringsCI locStringCI = m_locStringsEntries.Begin();
		locStringCI != m_locStringsEntries.End(); 
		++locStringCI, ++index )
	{
		Bool doesKeyMatch = locStringCI->m_stringKey == key;
		if ( doesKeyMatch && index != baseIndex )
		{
			return true;
		}
	}

	return false;
}

String CLocalizedStringsWithKeys::GenerateUniqueKey( const String &baseKey )
{
	String uniqueKey;
	Int32 i = 0;

	do
	{
		uniqueKey = baseKey + TXT("_") + ToString( i );
		++i;
	} while( DoesKeyExist( -1, uniqueKey ) );

	return uniqueKey;
}
