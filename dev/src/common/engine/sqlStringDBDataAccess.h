/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_STRING_DB
#include "stringDBDataAccess.h"

class sqlStringDbConnection;


class CSQLStringDBDataAccess : public IStringDBDataAccess
{
	static const String SPEECH_DIRECTORY;
	static const String VOICEOVER_DIRECTORY;
	static const String VOICEOVER_EXTENSION;
	static const String LIPSYNC_DIRECTORY;

private:
	sqlStringDbConnection *m_SqlConnection;

public:
	CSQLStringDBDataAccess();
	virtual ~CSQLStringDBDataAccess();


public:
	virtual void Read( LocalizedString *localizedString );
	virtual void Update( TDynArray< LocalizedStringEntry >& stringEntries );
	virtual void Update( TDynArray< LocalizedString* > strings );
	virtual Bool Update( LocalizedString& localizedString ) override;
	virtual void Update( const THashMap< Uint32, String > &strings, const String &lang );
	virtual void UpdateStringInfos( TDynArray< LocalizedStringEntry >& stringEntries );

public:
	virtual void ReadLanguageList( THashMap< String, Uint32 >& availableLanguages );
	virtual void ReadAllStringsWithKeys( TDynArray< Uint32 > &stringsIds /* out */, TDynArray< String > &stringsKeys /* out */, TDynArray< String > &stringsCategories /* out */ ) ;
	virtual void ReadAllStringsCategories( TDynArray< String > &stringsCategories /* out */, Bool keysOnly /* in */  );

public:
	virtual String GetLocalizedText( Uint32 stringId, const String& locale, Bool *fallback = NULL );
	virtual String GetLocalizedText( Uint32 stringId, Uint32 locale, Bool *fallback = NULL );
	virtual String GetLocalizedTextByStringKey( const String &stringKey, const String& locale, Bool *fallback = NULL );
	virtual String GetLocalizedTextByStringKey( const String &stringKey, Uint32 locale, Bool *fallback = NULL );

public:
	virtual void ReadLanguagePack( Uint32 stringId, LanguagePack& pack, Bool immediate, Bool& textLoaded, Bool& voiceLoaded );
	virtual Bool ReadLanguagePackByStringKey( const String &stringKey, LanguagePack& pack, Bool immediate = false );
	virtual void ReadLanguagePacksForCook( Uint32 locale, const String& stringsView, THashMap< Uint32, LanguagePack* >& packs );

public:
	virtual LanguagePack* GetLanguagePack( Uint32 stringId, Bool textOnly, Uint32 locale );
	virtual void GetLanguagePackBatch( const TDynArray< Uint32 >& stringIds, Uint32 locale, TDynArray< TPair< Uint32, LanguagePack* > >& languagePacks );

public:
	virtual Bool DoesStringKeyExist( const String &stringKey );
	virtual Uint32 GetStringIdByStringKey( const String &stringKey );
	virtual String GetStringKeyById( Uint32 stringId );
	virtual String GetVOFilename( Uint32 stringId );
	
public:
	virtual void SearchForLocalizedStrings( const String& searchQuery, const TDynArray< String >& categories, TDynArray< Uint32 >& ids, TDynArray< String >* keys = NULL, TDynArray< String >* strings = NULL, Bool searchKeys = false, const Char* orderByCol = NULL );

public:
	virtual Bool ShouldCache() { return true; }
	virtual void Reconnect();
	virtual bool IsSQLConnectionValid() {return GetNextId() > 0;}

protected:
	void Connect();
	virtual Uint32 GetNextId();
	virtual Bool IsNewString( Uint32 stringId );

	Bool FillLanguagePackResources( LanguagePack &pack, Uint32 locale );
	
	Bool InsertStringInfo( const LocalizedStringEntry& entry ) const;
	Bool UpdateStringInfo( const LocalizedStringEntry& entry ) const;
};

#endif // #ifndef RED_FINAL_BUILD
