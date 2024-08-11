/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "localizedStringsWithKeysCache.h"
#include "localizationManager.h"

void CLocalizedStringsWithKeysCache::InitializeCache()
{
	ClearCache();

	//TDynArray< Uint32 > stringsIds;
	//TDynArray< String > stringsKeys;
	//TDynArray< String > stringsCategories;
	//SLocalizationManager::GetInstance().ReadAllStringsWithStringKeys( stringsIds, stringsKeys, stringsCategories );

	//for ( Uint32 i = 0; i < stringsIds.Size(); ++i )
	//{
		//AddLocStringEntryCache( stringsIds[i], stringsKeys[i], stringsCategories[i] );
	//}
}

String CLocalizedStringsWithKeysCache::GetCachedStringByKey( const String &stringKey )
{
	if ( stringKey == String::EMPTY ) return String::EMPTY;

	const String *stringValue = m_cacheStrings.FindPtr( stringKey );

	if ( stringValue )
	{
		return *stringValue;
	}
	else
	{
		String stringValue = SLocalizationManager::GetInstance().GetStringByStringKey( stringKey );
		m_cacheStrings.Insert( stringKey, stringValue );
		return stringValue;
	}
}

Bool CLocalizedStringsWithKeysCache::AddLocStringEntryCache( Uint32 stringId, const String &stringKey, const String &stringCategory )
{
	if ( stringId == 0 ) return false;
	if ( stringKey == String::EMPTY ) return false;


	LocalizedString locString;
	locString.SetIndex( stringId );

	String stringValue = locString.GetString();
	if ( stringValue == String::EMPTY ) return false;

	m_cacheStrings.Insert( stringKey, stringValue );

	return true;
}

void CLocalizedStringsWithKeysCache::ClearCache()
{
	// Discard all changes
	m_cacheStrings.Clear();
}
