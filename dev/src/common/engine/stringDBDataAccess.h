/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "localizableObject.h"

class LanguagePack;

class IStringDBDataAccess
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	virtual ~IStringDBDataAccess(){};

public:
	virtual void Read( LocalizedString *localizedString ) = 0;
	virtual void Update( TDynArray< LocalizedStringEntry >& stringEntries ) = 0;
	virtual void Update( TDynArray< LocalizedString* > strings ) = 0;
	virtual Bool Update( LocalizedString& localizedString ) = 0;
	virtual void Update( const THashMap< Uint32, String > &strings, const String &lang ) = 0;
	virtual void UpdateStringInfos( TDynArray< LocalizedStringEntry >& stringEntries ) = 0;

public:
	virtual void ReadLanguageList( THashMap< String, Uint32 >& availableLanguages ) = 0;
	virtual void ReadAllStringsWithKeys( TDynArray< Uint32 > &stringsIds /* out */, TDynArray< String > &stringsKeys /* out */, TDynArray< String > &stringsCategories /* out */ ) = 0;
	virtual void ReadAllStringsCategories( TDynArray< String > &stringsCategories /* out */, Bool keysOnly /* in */  ) = 0;

public:
	virtual String GetLocalizedText( Uint32 stringId, const String& locale, Bool *fallback = NULL ) = 0;							
	virtual String GetLocalizedText( Uint32 stringId, Uint32 locale, Bool *fallback = NULL ) = 0;									
	virtual String GetLocalizedTextByStringKey( const String &stringKey, const String& locale, Bool *fallback = NULL ) = 0;		
	virtual String GetLocalizedTextByStringKey( const String &stringKey, Uint32 locale, Bool *fallback = NULL ) = 0;				

public:
	virtual void ReadLanguagePack( Uint32 stringId, LanguagePack& pack, Bool immediate, Bool& textLoaded, Bool& voiceLoaded ) = 0;	// Deprecated
	virtual Bool ReadLanguagePackByStringKey( const String &stringKey, LanguagePack& pack, Bool immediate = false ) = 0;			// Deprecated
	virtual void ReadLanguagePacksForCook( Uint32 locale, const String& stringsView, THashMap< Uint32, LanguagePack* >& packs ) = 0;

public:
	virtual LanguagePack* GetLanguagePack( Uint32 stringId, Bool textOnly, Uint32 locale ) = 0;
	virtual void GetLanguagePackBatch( const TDynArray< Uint32 >& stringIds, Uint32 locale, TDynArray< TPair< Uint32, LanguagePack* > >& languagePacks ) = 0;

public:
	virtual Bool DoesStringKeyExist( const String &stringKey ) = 0;
	virtual Uint32 GetStringIdByStringKey( const String &stringKey ) = 0;
	virtual String GetStringKeyById( Uint32 stringId ) = 0;
	virtual String GetVOFilename( Uint32 stringId ) = 0;

public:
	virtual void SearchForLocalizedStrings( const String& searchQuery, const TDynArray< String >& categories, TDynArray< Uint32 >& stringIds, TDynArray< String >* keys = NULL, TDynArray< String >* strings = NULL, Bool searchKeys = false, const Char* orderByCol = NULL ) {};

public:
	virtual Bool ShouldCache() = 0;
	virtual void Reconnect(){};

	virtual IStringDBDataAccess* DecorateDataAccess( IStringDBDataAccess* decorator ) { return this; }

public:
	virtual Bool RetrieveLanguagePackVoice( Uint32 stringId, LanguagePack* pack ) { return false; }
	virtual bool IsSQLConnectionValid() {return true;}
protected:
	//! Returns -1 on failure
	virtual Uint32 GetNextId() = 0;


	void ApplyStringDBId( Uint32 newId, LocalizedString *localizedString )
	{
		if ( localizedString != NULL )
		{
			localizedString->SetIndex( newId );
		}
	}

};
