/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "stringDBDataAccess.h"

class CNullStringDBDataAccess : public IStringDBDataAccess
{
public:
	CNullStringDBDataAccess() {};
	virtual ~CNullStringDBDataAccess() {};

public:
	virtual void Read( LocalizedString *localizedString ) {}
	virtual void Update( TDynArray< LocalizedStringEntry >& stringEntries ) {}
	virtual void Update( TDynArray< LocalizedString* > strings ) {}
	virtual Bool Update( LocalizedString& localizedString ) override { return true; }
	virtual void Update( const THashMap< Uint32, String > &strings, const String &lang ) {}
	virtual void UpdateStringInfos( TDynArray< LocalizedStringEntry >& stringEntries ) {}

public:
	virtual void ReadLanguageList( THashMap< String, Uint32 >& availableLanguages ) {}
	virtual void ReadAllStringsWithKeys( TDynArray< Uint32 > &stringsIds /* out */, TDynArray< String > &stringsKeys /* out */, TDynArray< String > &stringsCategories /* out */ ) {}
	virtual void ReadAllStringsCategories( TDynArray< String > &stringsCategories /* out */, Bool keysOnly /* in */  ) {}

public:
	virtual String GetLocalizedText( Uint32 stringId, const String& locale, Bool *fallback = NULL ) { return String::EMPTY; }
	virtual String GetLocalizedText( Uint32 stringId, Uint32 locale, Bool *fallback = NULL ) { return String::EMPTY; }
	virtual String GetLocalizedTextByStringKey( const String &stringKey, const String& locale, Bool *fallback = NULL ) { return String::EMPTY; }
	virtual String GetLocalizedTextByStringKey( const String &stringKey, Uint32 locale, Bool *fallback = NULL ) { return String::EMPTY; }

public:
	virtual void ReadLanguagePack( Uint32 stringId, LanguagePack& pack, Bool immediate, Bool& textLoaded, Bool& voiceLoaded ) {}
	virtual Bool ReadLanguagePackByStringKey( const String &stringKey, LanguagePack& pack, Bool immediate = false ) { return true; }
	virtual void ReadLanguagePacksForCook( Uint32 locale, const String& stringsView, THashMap< Uint32, LanguagePack* >& packs ) {}

public:
	virtual LanguagePack* GetLanguagePack( Uint32 stringId, Bool textOnly, Uint32 locale ) { return NULL; }
	virtual void GetLanguagePackBatch( const TDynArray< Uint32 >& stringIds, Uint32 locale, TDynArray< TPair< Uint32, LanguagePack* > >& languagePacks ) {}

public:
	virtual Bool DoesStringKeyExist( const String &stringKey ) { return true; }
	virtual Uint32 GetStringIdByStringKey( const String &stringKey ) { return 0; }
	virtual String GetStringKeyById( Uint32 stringId ) { return String::EMPTY; }
	virtual String GetVOFilename( Uint32 stringId ) { return String::EMPTY; }

public:
	virtual Bool ShouldCache() { return false; }
	virtual void Reconnect() {};

protected:
	virtual Uint32 GetNextId() { return 0; }
	virtual Bool IsNewString( Uint32 stringId ) { return false; }
};
