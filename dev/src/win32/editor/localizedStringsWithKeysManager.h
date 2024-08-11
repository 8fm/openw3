// Copyright © 2015 CD Projekt Red. All Rights Reserved.

#pragma once
#include "../../common/engine/localizableObject.h"

/*
REMARKS
Deleting strings is being done by setting STRING_KEY for NULL value.
*/

// Localized Strings Manager - used by Localized Strings Editor
class CLocalizedStringsWithKeys : public ILocalizableObject
{
public:
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings );

	struct SLocStringWithKeyEntry
	{
		LocalizedString	m_locString;
		String			m_stringKey;
		Int32				m_categoryIndex;
		Bool			m_isModified;   // if true, than struct should be written to the data base on save
		Bool			m_isMarkedForRemove;
	};

	void Initialize();

	// Creating methods
	Int32 CreateLocStringEntry( const String &stringText, const String &stringKey, const String &stringCategory );

	// Updating methods
	Bool UpdateLocStringCategory( Uint32 stringIndex, const String &stringCategory );
	Bool UpdateLocStringText( Uint32 stringIndex, const String &stringText );
	Bool UpdateLocStringKey( Uint32 stringIndex, const String &stringKey );
	Int32 UpdateCategory( const String& category ); // returns index (always good), adds category if doesn't exist

	// Removing methods
	Bool RemoveLocString( Uint32 stringIndex );

	// Info methods
	Int32 GetCategoryIndex( const String& category ); // returns -1 if category doesn't exist
	Uint32 GetSize() const;
	LocalizedString GetLocString( Uint32 index ) const;
	String GetStringKey( Uint32 index ) const;
	String GetStringCategory( Uint32 index ) const;
	Uint32 GetStringIndex( Uint32 index ) const;
	Bool DoesMatchStringCategory( Uint32 index, const String &category ) const;
	Bool IsMarkedForDelete( Uint32 index ) const;
	const TDynArray< String >& GetCategories() { return m_locStringsCategories; }
	Bool DoesKeyExist( Int32 baseIndex, const String &key );

	// Utility methods
	String GenerateUniqueKey( const String &baseKey );

protected:
	Bool AddLocStringEntry( Uint32 stringId, const String &stringKey, const String &stringCategory ); // returns true on success
	Bool UpdateLocStringEntry( Uint32 stringId, const String &stringText, const String &stringKey, const String &stringCategory ); // returns true if something was updated
	Bool DoesLocStringEntryExist( Uint32 stringId, const String &stringKey ) const; // returns true if there is entry with stringId OR stringKey
	Bool DoesLocStringEntryExist( Uint32 stringId ) const; // returns true if there is entry with stringId
	Bool DoesLocStringEntryExist( const String &stringKey ) const; // returns true if there is entry with stringKey
	Int32  FindLocStringEntryById( Uint32 stringId );

protected:
	TDynArray< String >                 m_locStringsCategories;
	TDynArray< SLocStringWithKeyEntry > m_locStringsEntries;
};
